# SPDX-FileCopyrightText: 2021 James Choi
#
# SPDX-License-Identifier: Apache-2.0

if have_doxygen
doc-local:
	@$(DOXYGEN) Doxyfile
	@echo  The documentation is under: $(builddir)/src/bf_rt/docs/
else
doc-local:
	@echo "ERROR: unable to generate documentation; DOXYGEN not found!"
	@exit -1
endif
