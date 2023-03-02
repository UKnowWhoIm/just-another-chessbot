import logging
from typing import Tuple, List

import chess
import chess.engine
from chess import Termination

class MatchUp:
    white: chess.engine.SimpleEngine
    black: chess.engine.SimpleEngine
    white_time_limit = 5
    black_time_limit = 5

    def play_game(self) -> Tuple[bool, List[str]]:
        try:
            board = chess.Board()
            moves = []
            is_white = True
            while not board.is_game_over():
                if is_white:
                    result = self.white.play(board, chess.engine.Limit(time=self.white_time_limit))
                else:                
                    result = self.black.play(board, chess.engine.Limit(time=self.black_time_limit))
                if result.move is None:
                    break
                moves.append(result.move.uci())
                board.push(result.move)
                is_white = not is_white
            outcome = board.outcome()
            if outcome is None:
                # Resigned ig
                logging.info("Resigned Board: %s", board.fen())
                logging.info(moves)
                return not is_white, moves
            if outcome.termination == Termination.CHECKMATE:
                return outcome.winner, moves
        except chess.engine.EngineError as exc:
            logging.info(moves)
            raise exc
        finally:
            self.white.quit()
            self.black.quit()
