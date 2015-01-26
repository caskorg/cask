sparsegrind
===========

Analyse sparse matrices for possible optimisations.

## Usage

```
➜  sparsegrind git:(master) ✗ python main.py
usage: main.py [-h] [-f {mm,csr,coo,matlabtl}]
               [-a {sparsity,range,storage,changes,reordering,compression}]
               [-t TIMESTEP] [-r]
               file
```

## Requires

```
pip install --user scipy
pip install --user nosetests
pip install --user networkx
pip install --user coverage
```
