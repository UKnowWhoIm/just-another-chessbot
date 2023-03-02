import chess.engine
from base import MatchUp

if __name__ == "__main__":
    match_up = MatchUp()
    match_up.white = chess.engine.SimpleEngine.popen_uci("/app/engines/chess.out")
    match_up.black = chess.engine.SimpleEngine.popen_uci("/app/engines/chess.out")
    res = match_up.play_game()
    print(res[0])
    print(res[1])