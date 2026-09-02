FROM buildpack-deps:bookworm AS build-env

# Set the timezone
ENV TZ=America/New_York
RUN ln -snf /usr/share/zoneinfo/$TZ /etc/localtime && echo $TZ > /etc/timezone

# Configure env
ARG SECRET_KEY
ARG DB_HOST
ARG DB_PORT
ARG DB_NAME
ARG DB_USER
ARG DB_PASSWORD

ENV SECRET_KEY=$SECRET_KEY
ENV DB_HOST=$DB_HOST
ENV DB_PORT=$DB_PORT
ENV DB_NAME=$DB_NAME
ENV DB_USER=$DB_USER
ENV DB_PASSWORD=$DB_PASSWORD

# Update and install only build/runtime dependencies used by this service.
RUN sed -i 's|http://deb.debian.org|https://deb.debian.org|g; s|http://security.debian.org|https://security.debian.org|g' /etc/apt/sources.list.d/debian.sources && \
    apt-get update && \
    DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends \
      cmake pkg-config curl libjsoncpp-dev uuid-dev libpqxx-dev \
      libssl-dev zlib1g-dev libbz2-dev liblzma-dev libpq-dev && \
    apt-get clean && rm -rf /var/lib/apt/lists/*

# Clone the Drogon repository
RUN git clone https://github.com/drogonframework/drogon

# Build and install the Drogon library
RUN cd drogon && \
    git submodule update --init && \
    mkdir build && cd build && \
    cmake -DCMAKE_BUILD_TYPE=Release .. && \
    make && make install

# Clone JWT-CPP repository
RUN git clone https://github.com/Thalhammer/jwt-cpp.git

# Build and install the JWT-CPP library
RUN cd jwt-cpp && mkdir build && cd build && cmake .. && make && make install


# Copy the application code
COPY . .

# Install brcrypt
RUN git clone https://github.com/hilch/Bcrypt.cpp.git

# Run scripts
RUN chmod +x scripts -R
RUN ./scripts/create_dot_env.sh
RUN ./scripts/create_model_json.sh

# Build app out of source so build artifacts stay isolated and CTest can use
# the standard build directory.
RUN cmake -S . -B build -DCMAKE_BUILD_TYPE=Release && \
    cmake --build build --parallel && \
    chmod +x build/drogon_user_service

# Expose port 8000 for the app
EXPOSE 8000

# Start the app
CMD ["./build/drogon_user_service", "--action=run-server"]
