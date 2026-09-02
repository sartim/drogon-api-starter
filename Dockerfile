FROM buildpack-deps:bookworm AS build-env

ENV TZ=UTC
WORKDIR /src

# Keep source dependencies reproducible. Update these revisions deliberately,
# validate the Docker/CI matrix, then record the change in the PR.
ARG DROGON_TAG=v1.9.9
ARG DROGON_COMMIT=38dd5fea31a7a2727c0a6f6b6b04252374796cab
ARG JWT_CPP_TAG=v0.7.1
ARG JWT_CPP_COMMIT=e71e0c2d584baff06925bbb3aad683f677e4d498
ARG BCRYPT_CPP_COMMIT=0d18b6a99e8c57627910db4ef9a7706c009b12ad

RUN apt-get update && \
    DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends \
      cmake pkg-config curl libjsoncpp-dev uuid-dev libpqxx-dev libhiredis-dev \
      libssl-dev zlib1g-dev libbz2-dev liblzma-dev libpq-dev && \
    rm -rf /var/lib/apt/lists/*

RUN git clone --depth 1 --branch ${DROGON_TAG} --recurse-submodules https://github.com/drogonframework/drogon.git /tmp/drogon && \
    test "$(git -C /tmp/drogon rev-parse HEAD)" = "${DROGON_COMMIT}" && \
    cmake -S /tmp/drogon -B /tmp/drogon/build \
      -DCMAKE_BUILD_TYPE=Release \
      -DBUILD_POSTGRESQL=ON -DBUILD_REDIS=ON -DBUILD_MYSQL=OFF -DBUILD_SQLITE=OFF \
      -DBUILD_EXAMPLES=OFF -DBUILD_CTL=OFF && \
    cmake --build /tmp/drogon/build --parallel && \
    cmake --install /tmp/drogon/build

RUN git clone --depth 1 --branch ${JWT_CPP_TAG} https://github.com/Thalhammer/jwt-cpp.git /tmp/jwt-cpp && \
    test "$(git -C /tmp/jwt-cpp rev-parse HEAD)" = "${JWT_CPP_COMMIT}" && \
    cmake -S /tmp/jwt-cpp -B /tmp/jwt-cpp/build \
      -DCMAKE_BUILD_TYPE=Release && \
    cmake --build /tmp/jwt-cpp/build --parallel && \
    cmake --install /tmp/jwt-cpp/build

COPY . .
RUN git clone --depth 1 https://github.com/hilch/Bcrypt.cpp.git Bcrypt.cpp && \
    test "$(git -C Bcrypt.cpp rev-parse HEAD)" = "${BCRYPT_CPP_COMMIT}"

ARG ENABLE_USER_SERVICE=OFF
RUN cmake -S . -B build -DCMAKE_BUILD_TYPE=Release \
      -DENABLE_USER_SERVICE=${ENABLE_USER_SERVICE} && \
    cmake --build build --parallel && \
    ctest --test-dir build --output-on-failure

# Always compile the complete batteries-included profile in CI so changes to
# the optional service are validated even when the default image is minimal.
RUN cmake --preset user-service && \
    cmake --build --preset user-service --parallel && \
    ctest --preset user-service

FROM debian:bookworm-slim AS runtime

ENV TZ=UTC
WORKDIR /app

RUN apt-get update && \
    DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends \
      ca-certificates curl libbrotli1 libc-ares2 libgcc-s1 libjsoncpp25 \
      libpq5 libpqxx-6.4 libhiredis0.14 libssl3 libstdc++6 libuuid1 zlib1g && \
    rm -rf /var/lib/apt/lists/* && \
    useradd --system --create-home --home-dir /app --shell /usr/sbin/nologin appuser

COPY --from=build-env /usr/local/lib/ /usr/local/lib/
COPY --from=build-env /src/build/drogon_user_service /app/drogon_user_service
COPY --from=build-env /src/docs /app/docs
COPY scripts/docker-entrypoint.sh /app/docker-entrypoint.sh

RUN ldconfig && chmod 0755 /app/docker-entrypoint.sh /app/drogon_user_service && \
    chown -R appuser:appuser /app

USER appuser
EXPOSE 8000
HEALTHCHECK --interval=10s --timeout=5s --retries=5 --start-period=10s \
  CMD curl --fail --silent http://127.0.0.1:8000/health || exit 1

ENTRYPOINT ["/app/docker-entrypoint.sh"]
CMD ["/app/drogon_user_service", "--action=run-server"]
