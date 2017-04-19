#include <io.h>
#include <fcntl.h>
#include <windows.h>
#include "libbb.h"
#include "termios.h"


#define	ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x4
int
ansimode(void)
{
	HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);

	return SetConsoleMode(h, ENABLE_PROCESSED_OUTPUT |
				ENABLE_VIRTUAL_TERMINAL_PROCESSING);
}

void
coninit(void)
{
	DWORD mode_flags;

	_setmode(_fileno(stdin), _O_BINARY);
	if (GetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), &mode_flags)) {
//		fprintf(stderr, "Have mode flags %x\n", mode_flags);
		mode_flags  &= ~(ENABLE_PROCESSED_INPUT | ENABLE_MOUSE_INPUT);
		mode_flags = ENABLE_WINDOW_INPUT|ENABLE_VIRTUAL_TERMINAL_PROCESSING;
		SetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), mode_flags);
	}
}
int
tcsetattr(int fd, int mode,  const struct termios *t)
{
	HANDLE h;
	DWORD mode_flags;

	if (fd == 0)
		h = GetStdHandle(STD_INPUT_HANDLE);
	else if (fd == 1)
		h = GetStdHandle(STD_OUTPUT_HANDLE);
	else if (fd == 2)
		h = GetStdHandle(STD_ERROR_HANDLE);
	else
		return (-1);

	if (!GetConsoleMode(h, &mode_flags))
		return (-1);

	if (fd == 0) {
		if (t->c_iflag & ICANON)
			mode_flags |= ENABLE_LINE_INPUT | ENABLE_PROCESSED_INPUT;
		else
			mode_flags &= ~(ENABLE_LINE_INPUT | ENABLE_PROCESSED_INPUT);
		if (t->c_iflag & ECHO)
			mode_flags |= ENABLE_ECHO_INPUT;
		else
			mode_flags &= ~ENABLE_ECHO_INPUT;
	}

	if (!SetConsoleMode(h, mode_flags))
		return (-1);
	return (0);
}

int
tcgetattr(int fd, struct termios *t)
{
	HANDLE h;
	DWORD mode_flags;

	if (fd == 0)
		h = GetStdHandle(STD_INPUT_HANDLE);
	else if (fd == 1)
		h = GetStdHandle(STD_OUTPUT_HANDLE);
	else if (fd == 2)
		h = GetStdHandle(STD_ERROR_HANDLE);
	else
		return (-1);

	GetConsoleMode(h, &mode_flags);

	t->c_iflag = 0;
	t->c_oflag = 0;
	if (mode_flags & ENABLE_LINE_INPUT)
		t->c_iflag |= ICANON;
	if(mode_flags & ENABLE_ECHO_INPUT)
		t->c_iflag |= ECHO;

	t->c_cc[VERASE] = '\177';
	return (0);
}

int64_t read_key(int fd, char *buf UNUSED_PARAM, int timeout)
{
	HANDLE cin = GetStdHandle(STD_INPUT_HANDLE);
	INPUT_RECORD record;
	DWORD nevent_out, mode;
	int ret = -1;
	char *s;

	if (fd != 0)
		error_msg_and_die("read_key only works on stdin");
	if (cin == INVALID_HANDLE_VALUE)
		return -1;
	GetConsoleMode(cin, &mode);
	SetConsoleMode(cin, 0);

	if (timeout > 0) {
		if (WaitForSingleObject(cin, timeout) != WAIT_OBJECT_0)
			goto done;
	}
	while (1) {
		if (!ReadConsoleInput(cin, &record, 1, &nevent_out))
			goto done;
		if (record.EventType != KEY_EVENT || !record.Event.KeyEvent.bKeyDown)
			continue;
		if (!record.Event.KeyEvent.uChar.AsciiChar) {
			DWORD state = record.Event.KeyEvent.dwControlKeyState;

			if (state & (RIGHT_CTRL_PRESSED|LEFT_CTRL_PRESSED) &&
			    (record.Event.KeyEvent.wVirtualKeyCode >= 'A' &&
			     record.Event.KeyEvent.wVirtualKeyCode <= 'Z')) {
				ret = record.Event.KeyEvent.wVirtualKeyCode & ~0x40;
				break;
			}

			switch (record.Event.KeyEvent.wVirtualKeyCode) {
			case VK_DELETE: ret = KEYCODE_DELETE; goto done;
			case VK_INSERT: ret = KEYCODE_INSERT; goto done;
			case VK_UP: ret = KEYCODE_UP; goto done;
			case VK_DOWN: ret = KEYCODE_DOWN; goto done;
			case VK_RIGHT:
				if (state & (RIGHT_CTRL_PRESSED|LEFT_CTRL_PRESSED)) {
					ret = KEYCODE_CTRL_RIGHT;
					goto done;
				}
				ret = KEYCODE_RIGHT;
				goto done;
			case VK_LEFT:
				if (state & (RIGHT_CTRL_PRESSED|LEFT_CTRL_PRESSED)) {
					ret = KEYCODE_CTRL_LEFT;
					goto done;
				}
				ret = KEYCODE_LEFT;
				goto done;
			case VK_HOME: ret = KEYCODE_HOME; goto done;
			case VK_END: ret = KEYCODE_END; goto done;
			case VK_PRIOR: ret = KEYCODE_PAGEUP; goto done;
			case VK_NEXT: ret = KEYCODE_PAGEDOWN; goto done;
			}
			continue;
		}
		if ( (record.Event.KeyEvent.uChar.AsciiChar & 0x80) == 0x80 ) {
			s = &record.Event.KeyEvent.uChar.AsciiChar;
			OemToCharBuff(s, s, 1);
		}
		ret = record.Event.KeyEvent.uChar.AsciiChar;
		break;
	}
 done:
	SetConsoleMode(cin, mode);
	return ret;
}

