TMP_PATH="/tmp/spark_html_doc"
HTML_DIR="html"
TRASH=latex

clean-tags:
	rm -f tags

tags:
	ctags include/ src/ -R *

doc:
	doxygen docs/doxygen.conf

clean-all-dist:
	rm -rf src/spmv/build/Spmv* src/spmv/build/*.class

# This should work, but we probably don't want to push
# documentation while the repo is private
#update-doc:
	#rm -rf ${TMP_PATH}
	#cp ${HTML_DIR} ${TMP_PATH} -R && rm -rf ${HTML_DIR}
	#git fetch
	#git checkout gh-pages
	#cp ${TMP_PATH}/* . -R
	#rm -rf ${TRASH}
	#git add .
	#git commit -m "Update documentation"
	#git push -u origin gh-pages
	#rm -rf ${TMP_PATH}
	#git checkout master

.PHONY: doc update-doc tags
