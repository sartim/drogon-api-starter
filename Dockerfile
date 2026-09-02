FROM ubuntu:24.04 AS build-env

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
RUN sed -i 's|http://archive.ubuntu.com|https://archive.ubuntu.com|g; s|http://security.ubuntu.com|https://security.ubuntu.com|g' /etc/apt/sources.list.d/ubuntu.sources && \
    apt-get update && \
    DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends \
      ca-certificates cmake g++ gcc git curl libjsoncpp-dev uuid-dev \
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

# Build app
RUN cmake . && make && chmod +x drogon_user_service

# Expose port 8000 for the app
EXPOSE 8000

# Start the app
CMD ["./drogon_user_service", "--action=run-server"]
