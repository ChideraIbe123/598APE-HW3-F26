# CS598APE HW3 - Abelian Sandpile

My optimized version of the sandpile identity code. On my machine (4 vCPUs of a Xeon Silver 4216) `./sandpile.exe 1001` went from 615 s to 21.5 s, about 28.6x faster. The output image is byte-for-byte the same as the original.

## Build and run

You just need gcc with OpenMP (I used gcc 15.2) and make.

```bash
make
./sandpile.exe 1001
```

It writes `identity.png`. By default it uses one thread fewer than the number of cores it can see. To pick the thread count yourself, set `OMP_NUM_THREADS`. My timings used 3 threads on cores 1-3:

```bash
OMP_NUM_THREADS=3 taskset -c 1-3 ./sandpile.exe 1001
```

Or with Docker:

```bash
docker build -t sandpile .
docker run --rm sandpile
```

## Checking it's still correct

The output should match the reference image exactly:

```bash
cmp identity.png identity-ref.png && echo same
```

## Benchmarking

Each optimization is its own commit, so you can check one out and time it. To time the original, check out `15fe837`. The original takes 10 minutes or so, so N=401 is a quicker way to compare commits.

```bash
git checkout <commit>
make clean && make
time ./sandpile.exe 1001
git checkout main
```

| # | Commit | Change | N=1001 time |
|---|--------|--------|-------------|
| 0 | `15fe837` | original | 615 s |
| 1 | `4a3be23` | `-O3 -flto -funroll-loops` + branch alignment flag | 360 s |
| 2 | `54bba0f` | store the pacman shape as one [lo, hi] range per row | 269 s |
| 3 | `4db5789` | padded grid, branch-free topple | 172 s |
| 4 | `8a66f68` | mirror symmetry, only compute the top half | 91 s |
| 5 | `bb339e8` | topple with `>> 2` and `& 3` | 64 s |
| 6 | `ffa222a` | process rows in pairs | 54 s |
| 7 | `852fa69` | OpenMP (3 threads) | 21.5 s |

All flags are in the `Makefile`.
