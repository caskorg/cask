doc:
	doxygen docs/doxygen.conf

update-doc:
	cp html /tmp/spark_html_doc -R
	git fetch
	git checkout gh-pages
	cp /tmp/spark_html_doc/* . -R
	git add .
	git commit -m "Update documentation"
	git push -u origin gh-pages


.PHONY: doc
