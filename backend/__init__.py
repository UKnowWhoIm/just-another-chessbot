"""
Setup logging
"""
import logging
import sys

LEVEL = logging.DEBUG 

logging.basicConfig(
    level=LEVEL,
    stream=sys.stdout,
)
