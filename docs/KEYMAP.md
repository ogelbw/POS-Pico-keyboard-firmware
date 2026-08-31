# Key map

The matrix is stored as `keyMap[col][row]`. Columns run left to right when you look at the keyboard face; rows run top to bottom.

Column pins: `{13, 12, 11, 10, 9, 8, 7, 21, 6, 5, 4, 3, 2, 1, 0}`

Row pins: `{20, 19, 18, 17, 16}`

A `.` marks a physical keypad position that does not exist as a key. Those read `HID_KEY_NONE`.

```
col  0      1      2      3      4      5      6      7      8      9      10     11     12     13     14
row0 Esc    1      2      3      4      5      6      7      8      9      0      -      =      PrSc   Bksp
row1 Tab    Q      W      E      R      T      Y      U      I      O      P      [      ]      /      \
row2 Caps   A      S      D      F      G      H      J      K      L      ;      '      `      Enter  .
row3 Shift  .      Z      X      C      V      B      N      M      ,      .      Shift  .      Up     App
row4 Ctrl   .      Alt    .      .      .      Space  .      .      .      AltR   Fn     Left   Down   Right
```

A few notes on that table:

- The layout is not a full rectangle near the bottom: column 1 (row 1 is `Q`) has no key on rows 3 and 4, and several cells in rows 3 and 4 are padding.
- Fn sits at column 11, row 4.
- The right-side modifiers and arrow cluster live at the bottom right: AltR (col 10, row 4), Left (col 12, row 4), Down (col 13, row 4), Right (col 14, row 4).

## Fn layer

Hold Fn and the base key maps to:

| Base key | Fn result         |
|----------|-------------------|
| 1..0     | F1..F10           |
| `-`      | F11               |
| `=`      | F12               |
| `[`      | Previous track    |
| `]`      | Next track        |
| `P`      | Play / Pause      |
| App      | Delete            |
| Esc      | Reboot the board  |

Keys with no Fn mapping pass through unchanged.

## Bad keys

`B`, `U`, `N`, and Up are prone to double-pressing from soldering. They get a 100 ms debounce window so the ghost press does not repeat. They live in `kb::bad_keys` in `kb/keymap.cpp`.

Changing any of these tables means editing `kb/keymap.cpp`. The unit tests in `tests/test_keymap.cpp` check the dimensions and a sample of cells, so a table that gets the wrong number of rows or shifts a known key will fail the build rather than surprise you mid-typing.
