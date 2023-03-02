FROM python:3.9-slim

ARG DEBUG=0

RUN apt-get update && apt-get install g++ --no-install-recommends -y

RUN if [ $DEBUG = 1 ]; then apt-get install gdb --no-install-recommends -y; fi

COPY requirements.txt .

RUN python -m pip install -r requirements.txt

COPY . /app

WORKDIR /app

RUN chmod +x entrypoint.sh && chmod +x ./engines/compile.sh && ./engines/compile.sh

CMD [ "./entrypoint.sh" ]