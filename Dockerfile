FROM buildpack-deps:bookworm AS build-env

ENV TZ=UTC
WORKDIR /src

RUN apt-get update && \
    DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends \
      cmake pkg-config curl libjsoncpp-dev uuid-dev libpqxx-dev \
      libssl-dev zlib1g-dev libbz2-dev liblzma-dev libpq-dev && \
    rm -rf /var/lib/apt/lists/*

RUN git clone --depth 1 --recurse-submodules https://github.com/drogonframework/drogon.git /tmp/drogon && \
    cmake -S /tmp/drogon -B /tmp/drogon/build \
      -DCMAKE_BUILD_TYPE=Release \
      -DBUILD_POSTGRESQL=ON -DBUILD_MYSQL=OFF -DBUILD_SQLITE=OFF \
      -DBUILD_EXAMPLES=OFF -DBUILD_CTL=OFF && \
    cmake --build /tmp/drogon/build --parallel && \
    cmake --install /tmp/drogon/build

RUN git clone --depth 1 https://github.com/Thalhammer/jwt-cpp.git /tmp/jwt-cpp && \
    cmake -S /tmp/jwt-cpp -B /tmp/jwt-cpp/build \
      -DCMAKE_BUILD_TYPE=Release && \
    cmake --build /tmp/jwt-cpp/build --parallel && \
    cmake --install /tmp/jwt-cpp/build

COPY . .
RUN git clone --depth 1 https://github.com/hilch/Bcrypt.cpp.git Bcrypt.cpp

RUN cmake -S . -B build -DCMAKE_BUILD_TYPE=Release && \
    cmake --build build --parallel && \
    ctest --test-dir build --output-on-failure

FROM debian:bookworm-slim AS runtime

ENV TZ=UTC
WORKDIR /app

RUN apt-get update && \
    DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends \
      ca-certificates curl libbrotli1 libc-ares2 libgcc-s1 libjsoncpp25 \
      libpq5 libpqxx-6.4 libssl3 libstdc++6 libuuid1 zlib1g && \
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
