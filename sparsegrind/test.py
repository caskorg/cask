import fetch
from linalg import grindlinalg
from tabulate import tabulate

l = fetch.fetch()
ms = l.getSpdLinearSystems()
ms.download()

allResults = []
headers = None
for m in ms.matrixList:
    results, headers = grindlinalg.runCgSolvers(m.file, m.rhsFile)
    for r in results:
        r.insert(0, m.name)
    allResults.extend(
        results
    )

headers.insert(0, 'matrix')
print tabulate(allResults, headers)
