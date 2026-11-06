FROM ubuntu:24.04
RUN apt-get update && apt-get install -y build-essential && rm -rf /var/lib/apt/lists/*
WORKDIR /app
COPY . .
RUN make
CMD ["./sandpile.exe", "1001"]
