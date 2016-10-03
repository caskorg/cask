sparsegrind
===========

Analyse sparse matrices for possible optimisations.

## Installation

```
virtualenv venv/ && source venv/bin/activate
pip install -r requirements.txt
```

## Usage

Command line:

```
$ python main.py
usage: main.py [-h] [-f {mm,csr,coo,matlabtl}]
                [-a {sparsity,range,storage,changes,reordering,compress_bcsrvi,reduce_precision,plot,summary}]
                               [-t TIMESTEP] [-e TOLERANCE] [-r] [-i IGNORE]
                                              file
```

Interactive (e.g. fetch properties and download some matrices):

```
In [1]: sparsegrind.fetch.fetch().head(2).download()
--> Using cached list_by_nnz.html ; to force refetch, use the -f option
Fetching JGD_Kocay/Trec3
100% [..................................................................................] 895 / 895
Extracting...
Trec3/Trec3.mtx

Fetching JGD_Kocay/Trec4
100% [..................................................................................] 902 / 902
Extracting...
Trec4/Trec4.mtx
```
