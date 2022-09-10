import logging
import json
from aiohttp.web import Application, Response
import socketio
import subprocess
from backend.redis_conn import start_connection, redis_conn

ENGINE_PATH = "/app/engines"
INITIAL_DATA = {"fen":"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 1 1"}
socket = socketio.AsyncServer()
app = Application()
socket.attach(app)

app.on_startup.append(start_connection)

@socket.event
def connect(sid, _):
    logging.info("Client: %s connected", sid)


@socket.on("chessMove")
async def chess_move(sid, move):
    logging.info("chess_move")
    data = (json.loads(redis_conn.client.get(sid).decode('utf-8'))['fen'], sid, move[0], move[1])
    logging.debug(data)
    process = subprocess.Popen([f"{ENGINE_PATH}/chess.out", *data], stdout=subprocess.PIPE)
    out = process.communicate()[0].decode("utf-8")

    try:
        [status, fen] = out.split(" ", maxsplit=1)
        redis_conn.client.set(sid, json.dumps({"fen": fen}))
        logging.debug(out)
        await socket.emit("chessResponse", {"data": {"fen": fen, "status": status}})
    except ValueError:
        logging.error(out)
        logging.error("Exception in C++ engine")
        await socket.emit("chessResponse", {"data": {"status": "EXC"}})


@socket.event
def disconnect(sid):
    redis_conn.client.delete(sid)
    logging.info("Disconnect: %s", sid)


@socket.on("startGame")
async def start_game(sid, player):
    redis_conn.client.set(sid, json.dumps(INITIAL_DATA))
    await socket.emit("setBoardState", INITIAL_DATA["fen"])

async def index(_):
    """Serve the client-side application."""
    with open('client/index.html') as f:
        return Response(text=f.read(), content_type='text/html')

async def chess(_):
    """Serve the chess application."""
    with open('client/chess.html') as f:
        return Response(text=f.read(), content_type='text/html')



app.router.add_get("/", index)
app.router.add_get("/chess", chess)
app.router.add_static("/static", "static")
