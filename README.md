# Deen

Deen is a simple German-English bi-lingual dictionary program.  It provides only a simple prefix-search, but that's actually fairly helpful.  The indexing process takes a very long time, but this only needs to happen initially.

## Contact / Author

Andrew Lindesay <apl@lindesay.co.nz> ([web](http://www.silvereye.co.nz/))

## License

Please see the LICENSE.TXT file for the license that governs the use of this software.

## Building

This software is designed to be built and run in a UN*X-like environment.  To build this software requires;

* C-Compiler
* Make
* SQL-Lite Version 3
* Flex

To build the software run the ```make``` command at the top level.  This will fairly quickly produce a ```deen``` executable.  If you want to get a debug build use ```make DEBUG=1```.

### Debian

Debian packages as prerequisites;

* ```sqlite3```
* ```libsqlite3-dev```
* ```flex```

### macOS

Install the Apple developers' tools.

### Haiku-OS

You will need to install sqlite3;

```
pkgman install sqlite_x86_devel
pkgman install sqlite_devel
```

### Installation

There is presently no _installation_ target so just use the resultant binary directly.

### Release

To release a Deen version, edit the version number in the Makefile.  To obtain a release version;

```
make clean
make
```

## Data

The data used with Deen comes from a project known as [Ding](https://www-user.tu-chemnitz.de/~fri/ding/).  You will need to download Ding's data to use Deen.  At the time of writing this data can be found [here](http://ftp.tu-chemnitz.de/pub/Local/urz/ding/de-en/de-en.txt.gz).  You will need to decompress the Ding data before use.

### Removal

The Deen application will tend to install a copy of the Ding data and its own index at ```~/.deen```.  If you no longer want to use Deen then you should delete this directory as well.

## Command Line Tool Usage

The ```deen``` command can be run with no arguments to see what options can be used with it.

### Installation

To install the data and index it, you need to run the ```deen``` tool as follows;

```
deen -i de-en.txt
```

This will take some time to complete.  It will output to the console to indicate what it is doing.

### Searching

To search for an entry, you run ```deen``` as follows;

```
deen Werkzeug
```

If you would like to find entries with two words then provide those as one argument by quoting them.

```
deen "astronaut launch"
```

In most modern terminals, the software should be able to cope with "umlaut characters" or the "scharfes S".  If your terminal doesn't support such characters, Deen can also handle abbreviations such as "ae" and "oe" (as in "Koenig") and will translate those latinizations to the corresponding accented characters.

Deen only shows a small number of the results.  Use the ```-c``` option to opt to show more or less results.
