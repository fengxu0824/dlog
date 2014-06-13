/*
  +----------------------------------------------------------------------+
  | PHP Version 5                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 1997-2013 The PHP Group                                |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.01 of the PHP license,      |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.php.net/license/3_01.txt                                  |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Author:                                                              |
  +----------------------------------------------------------------------+
*/

/* $Id$ */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_ini.h"
#include "ext/standard/info.h"
#include "php_dlog.h"

ZEND_DECLARE_MODULE_GLOBALS(dlog)

/* True global resources - no need for thread safety here */
static int le_dlog;

/* {{{ dlog_functions[]
 *
 * Every user visible function must have an entry in dlog_functions[].
 */
const zend_function_entry dlog_functions[] = {
	PHP_FE_END
};
/* }}} */

/* {{{ dlog_module_entry
 */
zend_module_entry dlog_module_entry = {
#if ZEND_MODULE_API_NO >= 20010901
	STANDARD_MODULE_HEADER,
#endif
	"dlog",
	dlog_functions,
	PHP_MINIT(dlog),
	PHP_MSHUTDOWN(dlog),
	NULL,
	NULL,
	PHP_MINFO(dlog),
#if ZEND_MODULE_API_NO >= 20010901
	DLOG_VERSION, 
#endif
	ZEND_MODULE_GLOBALS(dlog),
	PHP_GINIT(dlog),
	NULL,
	NULL,
	STANDARD_MODULE_PROPERTIES_EX
};
/* }}} */

#ifdef COMPILE_DL_DLOG
ZEND_GET_MODULE(dlog)
#endif

/* {{{ PHP_INI
 */
PHP_INI_BEGIN()
    STD_PHP_INI_ENTRY("dlog.enable", "0", PHP_INI_ALL, OnUpdateLong, enable, zend_dlog_globals, dlog_globals)
PHP_INI_END()
/* }}} */

error_callback old_error_callback = NULL;

void dlog_error_callback(
		int type
		,const char *filename
		,const uint lineno
		,const char *format
		,va_list args)
{
	TSRMLS_FETCH();

	if (DLOG_G(enable) && PG(log_errors) && ((EG(error_reporting) & type) || (type & E_CORE))) {
		char *dfilename = NULL
			,*dfunction_name = NULL
			,*dclass_name = NULL
			,*dclass_op = NULL
			,dline[DLOG_LINE_LEN];
		int dlineno = 0
			,dres = 0
			,dindex = 0;
		struct _zend_execute_data *edata = EG(current_execute_data);
		zval array;
		memset(dline, 0, DLOG_LINE_LEN);
		memset(&array, 0, sizeof(array));
		HashTable *val = Z_ARRVAL(array);

		while (edata) {
			if (edata->opline && (edata->opline->opcode == ZEND_DO_FCALL || edata->opline->opcode == ZEND_DO_FCALL_BY_NAME)) {
				if (!val) {
					array_init(&array);
				}
				zend_hash_next_index_insert(val, (void*)&edata, sizeof(void*), NULL);
			}
			edata = edata->prev_execute_data;
		}
		if (val) {
			Bucket *elem = val->pListTail;
			while (elem) {
				edata = *((struct _zend_execute_data**)(elem->pData));
				dfilename = edata->op_array->filename;
				dlineno = edata->opline->lineno;
				dfunction_name = edata->function_state.function->common.function_name;
				dclass_name = dclass_op = "";
				if (!dindex) {
						php_log_err("PHP Stack trace:" TSRMLS_CC);
				}
				if (edata->function_state.function->common.scope) {
					dclass_name = edata->function_state.function->common.scope->name;
					if (edata->object) {
						dclass_op = "->";
					} else {
						dclass_op = "::";
					}
				}
				dres = snprintf(dline
						,DLOG_LINE_LEN
						,"PHP Frame%d. %s%s%s() call at [%s:%d]"
						,dindex
						,dclass_name
						,dclass_op
						,dfunction_name
						,dfilename
						,dlineno);
				php_log_err(dline TSRMLS_CC);
				elem = elem->pListLast;
				dindex++;
			}
			zend_hash_destroy(val);
		}		
	}
	old_error_callback(type, filename, lineno, format, args);
}

PHP_GINIT_FUNCTION(dlog)
{
	DLOG_G(enable) = 0;
}

/* {{{ PHP_MINIT_FUNCTION
 */
PHP_MINIT_FUNCTION(dlog)
{
	REGISTER_INI_ENTRIES();
	old_error_callback = zend_error_cb;
	zend_error_cb = dlog_error_callback;
	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MSHUTDOWN_FUNCTION
 */
PHP_MSHUTDOWN_FUNCTION(dlog)
{
	UNREGISTER_INI_ENTRIES();
	zend_error_cb = old_error_callback;
	return SUCCESS;
}
/* }}} */


/* {{{ PHP_MINFO_FUNCTION
 */
PHP_MINFO_FUNCTION(dlog)
{
	DISPLAY_INI_ENTRIES();
	php_info_print_table_start();
	php_info_print_table_header(2, "dlog support", "enabled");
	php_info_print_table_row(2, "dlog version", DLOG_VERSION);
	php_info_print_table_row(2, "dlog author", "fengxu");
	php_info_print_table_end();

}
/* }}} */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker 
 * vim<600: noet sw=4 ts=4
 */
