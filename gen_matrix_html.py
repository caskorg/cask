import os
import shutil
from html import HTML

matrices = []

for root, dirs, files in os.walk('matrices'):
  h = HTML()
  matrix = os.path.basename(root)
  if not dirs:
    print root, dirs, files
    h.p('Matrix: ' + matrix)
    sparsity_plot = None
    for f in files:
      if not f.endswith('.png'):
        with open(os.path.join(root, f)) as fin:
          h.p(fin.read(), style='white-space: pre-wrap;')
      else:
        p = h.p()
        p.img(src=matrix + '.png')
        sparsity_plot = os.path.join(root, f)

    path = 'matrices_html/' + matrix + '.html'
    with open(path, 'w') as fout:
      matrices.append(matrix + '.html')
      fout.write(str(h))
      shutil.copyfile(sparsity_plot, 'matrices_html/' + matrix + '.png')

with open('matrices_html/index.html', 'w') as fout:
  h = HTML()
  h.p('matrices: ')
  l = h.ol

  for m in matrices:
    l.li.a(m, href=m)

  fout.write(str(h))
