FROM python:3.9-alpine3.14

RUN apk update && apk add g++ 

RUN apk --virtual add git && git clone https://github.com/cpp-redis/cpp_redis.git && \
cd cpp_redis && git submodule init && git submodule update && \
apk del git

RUN apk --virtual add cmake make \ 
    && cd cpp_redis && mkdir build && cd build && \
    cmake .. -DCMAKE_BUILD_TYPE=Release && make && make install && \
    apk del make cmake

COPY requirements.txt .

RUN python -m pip install -r requirements.txt

COPY . /app

WORKDIR /app

RUN chmod +x entrypoint.sh && chmod +x ./engines/compile.sh

CMD [ "./entrypoint.sh" ]