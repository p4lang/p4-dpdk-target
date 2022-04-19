UNITTEST_DIR = $(prefix)/unit_test_result
COVERAGE_DIR = $(prefix)/coverage_report
REPORT_DIR = $(prefix)/unit_test_report
xml_report:
	mkdir -p $(REPORT_DIR) && rm -f $(REPORT_DIR)/report.xml
	@for file in $(shell ls ${UNITTEST_DIR}/*); do $$file --gtest_output=xml:out.xml && cat out.xml >>  $(REPORT_DIR)/report.xml && rm out.xml; done
json_report:
	mkdir -p $(REPORT_DIR) && rm -f $(REPORT_DIR)/report.json
	@for file in $(shell ls ${UNITTEST_DIR}/*); do $$file --gtest_output=json:out.json && cat out.json >> $(REPORT_DIR)/report.json && rm out.json; done
runtest:
	@for file in $(shell ls ${UNITTEST_DIR}/*); do $$file && rm $$file; done
coverage:
	@$(shell mkdir -p $(COVERAGE_DIR); gcovr --filter ../src/ -e ".*.hpp" -e ".*.h" --exclude-throw-branches --exclude-unreachable-branches --html --html-details $(COVERAGE_DIR)/report.html)
all:
	mkdir -p ${UNITTEST_DIR} build 
	cd build && cmake -DCMAKE_INSTALL_PREFIX=$(prefix) -DCMAKE_ARGV0=$(dir) -DCMAKE_ARGV1=$(sub_dir) ..
	$(MAKE) -C build -j
install:
	echo "Nothing to be done in install"
clean:
	rm -rf build
	rm -rf $(UNITTEST_DIR)
	rm -rf $(COVERAGE_DIR)
	rm -rf $(REPORT_DIR)