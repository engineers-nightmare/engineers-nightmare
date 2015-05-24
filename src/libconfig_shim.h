#ifndef EN_LIBCONFIG_SHIM
#define EN_LIBCONFIG_SHIM
/* various shims around broken libconfig compat
 * http://upstream-tracker.org/versions/libconfig.html
 * is quite useful
 */

/* renamed: `config_lookup_from` to `config_setting_lookup`
 * http://upstream-tracker.org/diffs/libconfig/1.4.9_to_1.5/diff.html
 * with this change they bumped LIBCONFIG_VER_MINOR from 4 to 5
*/
#if LIBCONFIG_VER_MINOR < 5
#define config_setting_lookup(setting, path) config_lookup_from(setting, path)
#endif


/* LIBCONFIG_VER_MINOR was added in 1.4.5
 * http://upstream-tracker.org/diffs/libconfig/1.3.2_to_1.4.5/diff.html
*/
#ifndef LIBCONFIG_VER_MINOR

/* needed for config_setting_lookup, taken from
 * taken from libconfig.c : 48
 * f9f23d7a95608936ea7d839731dbd56f1667b7ed
 * https://github.com/hyperrealm/libconfig
 */

#ifndef PATH_TOKENS
#define PATH_TOKENS ":./"
#endif
/* config_setting_lookup doesn't exist in 1.3.2 which is in precise (12.04)
 * taken from libconfig.c : 1189
 * f9f23d7a95608936ea7d839731dbd56f1667b7ed
 * https://github.com/hyperrealm/libconfig
 */
config_setting_t *config_setting_lookup(config_setting_t *setting,
                                        const char *path)
{
  const char *p = path;
  config_setting_t *found;

  for(;;)
  {
    while(*p && strchr(PATH_TOKENS, *p))
      p++;

    if(! *p)
      break;

    if(*p == '[')
      found = config_setting_get_elem(setting, atoi(++p));
    else
      found = config_setting_get_member(setting, p);

    if(! found)
      break;

    setting = found;

    while(! strchr(PATH_TOKENS, *p))
      p++;
  }

  return(*p ? NULL : setting);
}
#endif


#ifndef config_error_file
/* `config_error_file` doesn't exist in 1.3.2 which is in precise (12.04) */
/* if we look at the macro `config_error_file`
 * from lib/libconfig.h : 314
 * f9f23d7a95608936ea7d839731dbd56f1667b7ed
 * https://github.com/hyperrealm/libconfig
 *
 * we see:
 *
 *    #define config_error_file(C) ((C)->error_file)
 *
 * however `error_file` was only added to `config_t` in 1.4.5
 * http://upstream-tracker.org/diffs/libconfig/1.3.2_to_1.4.5/diff.html
 *
 * `config_error_file` is expected to return a char* of the file that
 * the error occurded on, as per
 * http://www.hyperrealm.com/libconfig/libconfig_manual.html#index-config_005ferror_005ffile
 *
 * for now, if your version of libconfig is *that* old, you will get a hardcoded string
*/
#define config_error_file(C) "Unkown file"
#endif

#endif
