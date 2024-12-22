# Contributing

Be sure to read ALL of the following before submitting patches.

## Code style

The general code style is something of a bastardization of FreeBSD's
style(9) AKA Kernel Normal Form (KNF). Indentation is two spaces per
indent level. Tabs are strictly prohibited unless explicitly stated
otherwise.

Pointers are always referred to by `<type>* <name>`, for example:

    int
    func(void* foo) /* function declaration
    {
      int* bar;
      int* baz;
    }

Use one line per variable declaration if the values are of pointer
type. If the variables declared are not pointer types, a single line
may declare multiple variables.

All function declarations are of the form:

    int
    func(...)
    {
      ... body ...
    }

The requirements are that the return type is on a separate line, and
braces fall on the line after the parameter list.

(Tip: this allows you to search `^func\(` with grep or your editor to
find to any definition quickly if you don't use tags or anything.)

Functions with external linkage are declared like this:

    /**
     * module_func(arg1, arg2, ...):
     * Description of what module_func does, referring to arguments
     * as ${arg1}.
     *
     */
     int
     module_func(void*, int);

The identical comment appears in the C file where the function is
defined.

Static functions may have the above form of comment, or simply a
`/* Brief description of what the function does. */`
line before the function.

If size_t is needed, <stddef.h> should be included to provide it unless
<stdio.h>, <stdlib.h>, <string.h>, or <unistd.h> is already required.

If the C99 integer types (`uint8_t`, `int64_t`, etc.) are required,
<stdint.h> should be included to provide them unless <inttypes.h> is
already required.

The type `char` should only be used to represent human-readable
characters (input from users, output to users, pathnames, et cetera).
The type `char*` should normally be a NUL-terminated string.  The
types `signed char` and `unsigned char` should never be used; C99
integer types should be used instead.

Functions named `foo_free` should return `void`, and `foo_free(NULL)`
should have no effect.

In non-trivial code, comments should be included which describe in
English what is being done by the surrounding code with sufficient
detail that if the code were removed, it could be replaced based on
reading the comments without requiring any significant creativity.

Comments and documentation should be written in en-GB-oed; i.e., with
the 'u' included in words such as "honour", "colour", and "neighbour",
and the ending '-ize' in words such as "organize" and "realize".  The
Oxford (aka. serial) comma should be used in lists.  Quotation marks
should be placed logically, i.e., not including punctuation marks
which do not form a logical part of the quoted text.

## Commits

Rules for contribution:

  * 80-character column maximum.
  * The first line of a commit message should be 73 columns max.
  * Try to make commits self contained. One thing at a time.
    If it's a branch, squash the commits together to make one.
  * Always run tests. If benchmarks regress, give OS information,
    and we'll discuss.
  * Always reference the issue you're working on in the bug tracker
    in your commit message, and if it fixes the issue, close it.

Send me a patch however you please.

For multi-commit requests, I will often squash them into the smallest
possible logical changes and commit with author attribution.

### Notes on sign-offs and attributions, etc.

When you commit, **please use -s to add a Signed-off-by line**. I manage
the `Signed-off-by` line much like Git itself: by adding it, you make clear
that the contributed code abides by the source code license. I'm pretty
much always going to want you to do this.

I normally merge commits manually and give the original author attribution
via `git commit --author`. I also sign-off on it, and add an `Acked-by` field
which basically states "this commit is not totally ludicrous."

Other fields may be added in the same vein for attribution or other purposes
(`Suggested-by`, `Reviewed-by`, etc.)

## Hacker notes

N/A.
