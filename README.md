# libtreadmill

An implementation of Baker's Treadmill Garbage Collector.

Baker's Treadmill is an incremental, non-moving GC algorithm, trivially made real-time. Read about the algorithm [in this article][gc].

## Why?

This library is used by [TerrorVM][terrorvm], a Virtual Machine for dynamic
languages, and was extracted from it to keep the project modular and the
interface clear.

## Using it in your project

### Installing it as a submodule

    $ cd <your_project>
    $ mkdir -p deps
    $ git submodule add git://github.com/txus/libtreadmill deps/libtreadmill
    $ 

In your Makefile, add this to your `CFLAGS`:

    CFLAGS=<whatever> -Ideps/libtreadmill/include

And this to your LDFLAGS:

    LDFLAGS=<whatever> deps/libtreadmill/build/libtreadmill.a

Create a `gc` target in your Makefile:

```make
gc:
  $(MAKE) -C deps/libtreadmill
```

And finally add it as a dependency of your main target:

```make
all: gc <your> <other> <targets>
```

Done! :)

### Interface

(to be written)

## Development

To build libtreadmill and run its test suite:

    $ git clone git://github.com/txus/libtreadmill
    $ cd libtreadmill
    $ make

## Contributing

1. Fork it
2. Create your feature branch (`git checkout -b my-new-feature`)
3. Commit your changes (`git commit -am 'Added some feature'`)
4. Push to the branch (`git push origin my-new-feature`)
5. Create new Pull Request

[gc]: http://www.pipeline.com/~hbaker1/NoMotionGC.html
[terrorvm]: https://github.com/txus/terrorvm
