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

`libtreadmill` interfaces with your project in two points: your **objects** (or
values or however you call them) and some kind of **state** (to figure out the
rootset).

As easily seen [in the tests][tests], here's an example State and Object
structs:

```c
typedef struct state_s {
  TmStateHeader gc;
  ... // your own things
} State;

typedef struct object_s {
  TmObjectHeader gc;
  ... // your own things
} Object;
```

The first element in both structs **must** be a `TmStateHeader` for the state
and a `TmObjectHeader` for the object.

#### Interface with the State

Whenever you initialize a State object, you have to give it a TmRootsetFn
function pointer, which will accept a pointer to a TmStateHeader and return a
pointer to a Tm_DArray (a dynamic array used by libtreadmill), and implement the
function, like this for example:

```c
#include <treadmill/gc.h>
#include <treadmill/darray.h>

static inline Tm_DArray*
get_rootset(TmStateHeader *state_header)
{
  // Create the dynamic array that we'll return
  Tm_DArray *rootset = Tm_DArray_create(sizeof(TmObjectHeader*), 10);

  // Typecast to our own State struct
  State *state = (State*)state_header;

  /*
   * From the state, figure out the rootset, and for each object, do this:
   *   Tm_DArray_push(rootset, object);
   */

  return rootset;
}

State*
State_new()
{
  State *state = calloc(1, sizeof(State));
  // Assign the function 
  state->gc.rootset = get_rootset;
  return state;
}
```

#### Interface with your objects

As we've seen, an object's first element is a `TmObjectHeader`. The Treadmill
needs to know three things about your objects: **their size**, how to **release
them**, and how to **scan their pointers** to other objects.

To let the Treadmill know about the first, it's as easy as `sizeof(Object)`.

Regarding how to release them, there's a function pointer for that. An example
implementation would be:

```c
void
release_my_object(void *value)
{
  // call your own Object_destroy function or whatever
  Object_destroy((Object*)value);
}
```

Regarding how to scan their pointers, there's another function pointer for that,
with this example implementation:

```c
void
scan_my_pointers(TmHeap *heap, TmObjectHeader *object, TmCallbackFn callback)
{
  // Typecast to your own Object type
  Object *self = (Object*)object;

  /*
   * Figure out the  `self` points to:, and for each `obj` of them, do this:
   *   callback(heap, (TmObjectHeader*)obj);
   */
}
```

You're all set! This is all the Treadmill needs to know about your objects.

### Creating, using and destroying the Heap

To initialize the heap, do this:

```c
TmHeap *heap = TmHeap_new(
  (TmStateHeader*)state, // your initialized state object
  1000,                  // initial size of the heap
  1500,                  // growth rate in number of cells
  200,                   // scan every 200 allocations
  sizeof(Object),        // the size of your Objects
  release_my_object,     // a pointer to your release function
  scan_my_pointers       // a pointer to your function to scan pointers
);
```

Keep a reference to the heap wherever you deem best. Every time you need to
**allocate an object**, do this:

```c
Object *my_obj = (Object*)Tm_allocate(heap);
```

And finally, at the end of your program, remember to destroy the heap:

```c
TmHeap_destroy(heap);
```

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
[tests]: https://github.com/txus/libtreadmill/blob/master/tests/gc_tests.c
