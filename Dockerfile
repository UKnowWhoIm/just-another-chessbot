FROM python:3.9-alpine3.14

ARG DEBUG=0

RUN apk update && apk add g++

RUN if [ $DEBUG = 1 ]; then apk add gdb; fi

COPY requirements.txt .

RUN python -m pip install -r requirements.txt

COPY . /app

WORKDIR /app

RUN chmod +x entrypoint.sh && chmod +x ./engines/compile.sh && ./engines/compile.sh

CMD [ "./entrypoint.sh" ]