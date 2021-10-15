if have_doxygen
doc-local:
	@$(DOXYGEN) Doxyfile
	@echo  The documentation is under: $(builddir)/doc/
else
doc-local:
	@echo "ERROR: unable to generate documentation; DOXYGEN not found!"
	@exit -1
endif
