import logging
from aiohttp.web import Application, Response
import socketio
import subprocess
from backend.redis_conn import start_connection

ENGINE_PATH = "/app/engines"

socket = socketio.AsyncServer()
app = Application()
socket.attach(app)

@socket.event
def connect(sid, _):
    logging.info("Client: %s connected", sid)

@socket.event
async def chess_move(sid, data):
    logging.info("chess_move")
    import json
    x = json.dumps({"data": {"fen": "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 0"}})
    process = subprocess.Popen([f"{ENGINE_PATH}/chess.out", x], stdout=subprocess.PIPE)
    out = process.communicate()[0].decode("utf-8")
    logging.info(out)
    await socket.emit("chess_response", {"data": out})

@socket.event
def disconnect(sid):
    logging.info("Disconnect: %s", sid)


async def index(_):
    """Serve the client-side application."""
    with open('client/index.html') as f:
        return Response(text=f.read(), content_type='text/html')


app.router.add_get("/", index)
