
Introduction
============

Slimox is another experimental lossless data compression program (with simple archiver support), based on PPM model and LZP dictionary coding.

------

Implementation Details
======================

1. PPM Model

    The PPM model used in slimox is based on the article "[Implementing PPMC with Hash Tables](http://www.arturocampos.com/ac_ppmc.htm)" by Arturo San Emeterio Campos, with some optimizations. The context order is 4-2-1-0-(-1), where order-4 implemented with a hash table and other orders are arrays. To improve compression, I used escape method PPMXA instead of PPMC, and for highest order-4, I implemented a simple SSE model and used II (Information Inheritance) to achieve better compression.

2. LZP Dictionary Coding

    Since the highest order of PPM model is order-4, I use an order 8-4-2 LZP coder to reduce redundancy of higher orders. For simple implementation and faster encoding, I use escape character instead of flags to indicate literals and matches. All LZP outputs are passed to the PPM model. With LZP coding, slimox's compression ratio is closed to an order-5 PPM model, much better than normal gzip/bzip2 compression.

------

Benchmark
=========

Here is a simple benchmark with __bible.txt__, compared with gzip and bzip2.

<table border="1">
 <tr><td>program</td>             <td>size</td>        <td>encode-time</td>     <td>decode-time</td></tr>
 <tr><td>gzip</td>                <td>1191061</td>     <td>0.90s</td>           <td>0.09s</td></tr>
 <tr><td>bzip2</td>               <td>845635</td>      <td>1.73s</td>           <td>0.71s</td></tr>
 <tr><td>slimox (20130316)</td>   <td>776954</td>      <td>1.83s</td>           <td>1.92s</td></tr>
</table>

Build
=====

You can simply use `make` to automatically build the whole project. You can also compile each .c source with `gcc -msse2 -O3`.

Run
===

Run ./bin/slimox under project directory to see a detail usage of slimox.
