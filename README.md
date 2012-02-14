![logo](https://github.com/baskerville/keydouble/raw/master/logo/keydouble_logo.png)

This little X utility allows you to use ordinary keys as modifiers while keeping the ordinary behavior under simple key press/release circumstances.

## Configuration

You shall edit `kdlaunch`.

The **NATURAL_KEYCODE** variable defines the keycode of the ordinary key (`xev` can be used to obtain this information).
The **ARTIFICIAL_KEYCODE** variable defines the keycode sent when the ordinary key is not used as a modifier. 

`xmodmap` is used to map **NATURAL_KEYCODE** to the modifier keysym and **ARTIFICIAL_KEYCODE** to the ordinary keysym.

## Testing

    make
    ./kdlaunch

## Dependencies

- libxtst
- xorg-xmodmap
- dash

If needed (i.e. if you encounter errors), the `record` X module can be loaded with:

    Section "Module"
        Load "record"
    EndSection
