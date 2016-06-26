#import "core/types.h"
#import "core/common.h"
#import "core/keyword.h"

static void test_keywords_all_present() {
	deen_keywords *keywords = deen_keywords_create();

	deen_keywords_add_from_string(keywords, (uint8_t *) "YERT");

	if(DEEN_TRUE != deen_keywords_all_present(keywords, (uint8_t *) "zing {adj} | Zing Zong Ting | Yert")) {
		deen_log_error_and_exit("failed test 'test_keywords_all_present'");
	}

	deen_keywords_free(keywords);
}

int main(int argc, char** argv) {
	test_keywords_all_present();
	return 0;
}
