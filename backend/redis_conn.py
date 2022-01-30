from os import environ
import redis


class RedisClient:
    client: redis.Redis = None


redis_conn = RedisClient()


def start_connection():
    redis_conn.client = redis.Redis.from_url(environ["REDIS_URL"])
