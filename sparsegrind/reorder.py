from networkx.utils import reverse_cuthill_mckee_ordering
from networkx.utils import cuthill_mckee_ordering
import networkx as nx


def rcm(matrix):
    """Returns a reverse Cuthill-McKee reordering of the given matrix."""
    G = nx.from_scipy_sparse_matrix(matrix)
    rcm = reverse_cuthill_mckee_ordering(G)
    return nx.to_scipy_sparse_matrix(G, nodelist=list(rcm), format='csr')


def min_degree_heuristic(G):
        n, d = sorted(G.degree().items(), key=lambda x: x[1])[0]
        return n


def rcm_min_degree(matrix):
    """Returns a reverse Cuthill-McKee reordering of the given matrix,
    using the minimum degree heuristic."""
    G = nx.from_scipy_sparse_matrix(matrix)
    rcm = reverse_cuthill_mckee_ordering(G, heuristic=min_degree_heuristic)
    return nx.to_scipy_sparse_matrix(G, nodelist=list(rcm), format='csr')


def cm(matrix):
    """Returns a Cuthill-McKee reordering of the given matrix."""
    G = nx.from_scipy_sparse_matrix(matrix)
    rcm = cuthill_mckee_ordering(G)
    return nx.to_scipy_sparse_matrix(G, nodelist=list(rcm), format='csr')
