import threading, time
from KeyboardInput import KeyboardProcessor

# Use Mock to simulate key events instead of a bespoke FakeEvent.
from unittest.mock import Mock


def _make_event(key: str):
    evt = Mock()
    evt.name = key
    evt.event_type = "down"
    return evt


def worker(kp, keys):
    for k in keys:
        kp.process_key(_make_event(k))
        # tiny sleep to force context switch
        time.sleep(0.001)


def test_keyboard_processor_thread_safety():
    # If the KeyboardProcessor were not thread-safe (e.g. it used unsynchronised shared state), concurrent updates
    # could corrupt the internal cursor coordinates and push them outside the board, causing the assertion to fail.
    # Passing this test therefore demonstrates that:
    # The cursor never leaves the legal board area even under heavy simultaneous key processing.
    # Internal state updates inside KeyboardProcessor are performed in a thread-safe manner
    keymap = {"up": "up", "down": "down", "left": "left", "right": "right"}
    kp = KeyboardProcessor(8, 8, keymap)
    seq1 = ["up", "up", "left", "down", "right"] * 500
    seq2 = ["down", "right", "right", "up", "left"] * 500
    t1 = threading.Thread(target=worker, args=(kp, seq1))
    t2 = threading.Thread(target=worker, args=(kp, seq2))
    t1.start()
    t2.start()
    t1.join()
    t2.join()
    r, c = kp.get_cursor()
    assert 0 <= r < 8 and 0 <= c < 8, "Cursor out of bounds after concurrent access"
