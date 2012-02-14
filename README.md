![logo](https://github.com/baskerville/keydouble/raw/master/logo/keydouble_logo.png)

This little X utility allows you to use ordinary keys as modifiers while keeping the ordinary behavior under simple key press/release circumstances.

## Configuration

`keydouble` honors a configuration file at `~/.keydoublerc`.
See `example/keydoublerc`.

## Install

Run

    make

Then copy the executables to your `bin` directory.
And add

    kdlaunch &

to `~/.xinitrc`.


## Dependencies

- libxtst
- xorg-xmodmap
- dash

If needed (i.e. if you encounter errors), the `record` X module can be loaded with:

    Section "Module"
        Load "record"
    EndSection
