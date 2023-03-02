from datetime import datetime
import json
import logging
import chess
import chess.engine
from chess.engine import EngineError
from chess import Termination

logging.basicConfig(filename="/app/engines/tests/logs", level=logging.INFO)
games = 8
stats = {}


def play_game(is_white, attack, defense, space):
    board = chess.Board()
    is_engine = is_white
    try:
        engine = chess.engine.SimpleEngine.popen_uci("/app/engines/chess.out")
        engine.configure({"attackmultiplier": attack, "defencemultiplier": defense, "spacemultiplier": space})
        stockfish = chess.engine.SimpleEngine.popen_uci("/app/engines/tests/stockfish")
        stockfish.configure({"Skill Level": 0})
        moves = []
        while not board.is_game_over():
            if not is_engine:
                result = stockfish.play(board, chess.engine.Limit(time=0.1))
            else:
                try:
                    result = engine.play(board, chess.engine.Limit(time=10))
                except EngineError as exc:
                    raise Exception() from exc
            if result.move is None:
                break
            moves.append(result.move.uci())
            board.push(result.move)
            is_engine = not is_engine
        outcome = board.outcome()
        if outcome is None:
            # Resigned ig
            logging.info("Resigned Board: %s", board.fen())
            logging.info(moves)
            return not is_engine, moves
        if outcome.termination == Termination.CHECKMATE:
            return outcome.winner == is_white, moves
        return None, moves
    finally:
        engine.quit()
        stockfish.quit()



def game_overhead(attack, defence, space):
    win = 0
    loss = 0
    draw = 0
    game_length = 0
    for i in range(games):
        logging.info(i)
        try:
            start = datetime.now()
            game = play_game(i < games / 2, attack, defence, space)
            game_length += (datetime.now() - start).total_seconds()
            if game[0]:
                win += 1
                logging.info("Win")
            elif game[0] == False:
                loss += 1
                logging.info("Loss")
            else:
                draw += 1
                logging.info("Draw")
        except Exception:
            logging.error("Error")
        logging.info(f"Wins: {win}, Loss: {loss}, draw: {draw}")
    stats[f"{attack} {defence} {space}"] = (win, loss, draw, game_length / games)
    with open("stats.json", "w") as f:
        logging.info("Write to file")
        json.dump(stats, f)

for attack in range(8, 40):
    for defence in range(8, 40):
        for space in range(1, 15):
            game_overhead(attack, defence, space)

            