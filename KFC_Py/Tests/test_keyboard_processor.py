import pytest
from KeyboardInput import KeyboardProcessor

# Use a simple Mock object to emulate the keyboard event instead of a handcrafted fake.
from unittest.mock import Mock


def _make_event(key: str):
    evt = Mock()
    evt.name = key
    evt.event_type = "down"
    return evt

@pytest.fixture
def keymap1():
    return {
        'w': 'up',
        's': 'down',
        'a': 'left',
        'd': 'right',
        'enter': 'choose',
        '+': 'jump'
    }

def test_initial_position_and_get_cursor(keymap1):
    kp = KeyboardProcessor(8, 8, keymap1)
    assert kp.get_cursor() == (0, 0)

def test_cursor_moves_and_wraps(keymap1):
    kp = KeyboardProcessor(2, 3, keymap1)
    # up from (0,0) → stays (0,0)
    kp.process_key(_make_event('w'))
    assert kp.get_cursor() == (0, 0)
    # down → (1,0)
    kp.process_key(_make_event('s'))
    assert kp.get_cursor() == (1, 0)
    kp.process_key(_make_event('s'))  # down again → still (1,0)
    assert kp.get_cursor() == (1, 0)
    kp.process_key(_make_event('a'))  # left → (1,0) stays (0)
    assert kp.get_cursor() == (1, 0)
    kp.process_key(_make_event('d'))  # right → (1,1)
    assert kp.get_cursor() == (1, 1)

def test_choose_and_jump_return_actions(keymap1):
    kp = KeyboardProcessor(5, 5, keymap1)
    kp._cursor = [3, 4]
    choice = kp.process_key(_make_event('enter'))
    assert choice == 'choose'
    jump = kp.process_key(_make_event('+'))
    assert jump == 'jump'
    assert kp.process_key(_make_event('w')) == 'up'
    assert kp.process_key(_make_event('x')) is None  # unknown key

def test_custom_keymaps():
    km2 = {
        'i': 'up',
        'k': 'down',
        'j': 'left',
        'l': 'right',
        'o': 'choose',
        'p': 'jump'
    }
    kp = KeyboardProcessor(5, 5, km2)
    kp.process_key(_make_event('i'))  # up → (0,0) stays (0,0)
    assert kp.get_cursor() == (0, 0)
    kp.process_key(_make_event('k'))  # down → (1,0)
    assert kp.get_cursor() == (1, 0)
    kp._cursor = [2, 2]
    assert kp.process_key(_make_event('o')) == 'choose'
    assert kp.process_key(_make_event('p')) == 'jump'
