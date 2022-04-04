#include "test.h"
#include "ctest.h"

int main()
{
	int ret = 0;

	logger_set_level(LOGGER_LEVEL_OFF);
	describe("test_css_value", test_css_value);
	return ret - print_test_result();
}
