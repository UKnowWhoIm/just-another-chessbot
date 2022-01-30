#!/bin/sh
./engines/compile.sh
gunicorn backend.main:app  --reload --bind=0.0.0.0:${PORT:-8000} --worker-class aiohttp.GunicornWebWorker
#gunicorn --bind=0.0.0.0:${PORT:-8000} backend.main:app
