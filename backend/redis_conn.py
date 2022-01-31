from os import environ
import redis


class RedisClient:
    client: redis.Redis = None


redis_conn = RedisClient()


async def start_connection(_):
    redis_conn.client = redis.Redis.from_url(environ["REDIS_URL"])
