#ifndef _PWART_SYSLIB_WASI_C
#define _PWART_SYSLIB_WASI_C

#include <stdint.h>
#include <pwart.h>
#include <uv.h>
#include <uvwasi.h>
#include <stdlib.h>

#include "./util.c"

static struct pwart_wasm_memory *uwmem;
static uvwasi_t uvwc;
static int uvwcinited=0;

static struct dynarr *wasisyms = NULL; // type pwart_named_symbol

static void wasm__args_get(void *fp) { 
   void *sp = fp;
  _wargi32(argv) _wargi32(argv_buf) 
  memcpy(uwmem->bytes + argv_buf,uvwc.argv_buf,uvwc.argv_buf_size);
  int i;
  uint32_t *pargv=(uint32_t *)(uwmem->bytes + argv);
  for(i=0;i<uvwc.argc;i++){
    pargv[i]=(uint32_t)(uvwc.argv[i]-uvwc.argv_buf+argv_buf);
  }
  sp = fp;
  pwart_rstack_put_i32(&sp, 0);
}
static void wasm__args_sizes_get(void *fp) { 
  void *sp = fp;
  _wargi32(argc) _wargi32(argv_buf_size) uvwasi_errno_t err =
      uvwasi_args_sizes_get(&uvwc, (uvwasi_size_t *)(uwmem->bytes + argc),
                            (uvwasi_size_t *)(uwmem->bytes + argv_buf_size));
  sp = fp;
  pwart_rstack_put_i32(&sp, err);
}
static void wasm__clock_res_get(void *fp) { 
  void *sp = fp;
  _wargi32(clock_id) _wargi32(resolution) uvwasi_errno_t err =
      uvwasi_clock_res_get(&uvwc, clock_id, (uvwasi_timestamp_t *)(uwmem->bytes + resolution));
  sp = fp;
  pwart_rstack_put_i32(&sp, err);
}
static void wasm__clock_time_get(void *fp) { 
   void *sp = fp;
  _wargi32(clock_id) _wargi64(precision) _wargi32(time) uvwasi_errno_t err =
      uvwasi_clock_time_get(&uvwc, clock_id, precision, (uvwasi_timestamp_t *)(uwmem->bytes + time));
  sp = fp;
  pwart_rstack_put_i32(&sp, err);
}
static void wasm__environ_get(void *fp) { 
   void *sp = fp;
  _wargi32(environment) _wargi32(environ_buf) 
  uv_env_item_t *penv=NULL;
  int envcnt=0;
  uv_os_environ(&penv,&envcnt);
  char *buf=uwmem->bytes + environ_buf;
  uint32_t *wenv=(uint32_t *)(uwmem->bytes + environment);
  int i;
  for(i=0;i<envcnt;i++){
    wenv[i]=buf-(char *)(uwmem->bytes);
    strcpy(buf,penv[i].name);
    buf+=strlen(penv[i].name);
    *buf='=';
    buf+=1;
    strcpy(buf,penv[i].value);
    buf+=strlen(penv[i].value);
    *buf='\0';
    buf+=1;
  }
  sp = fp;
  pwart_rstack_put_i32(&sp, 0);
}
static void wasm__environ_sizes_get(void *fp) { 
   void *sp = fp;
  _wargi32(environ_count) _wargi32(environ_buf_size)
  uv_env_item_t *penv=NULL;
  int envcnt=0;
  uv_os_environ(&penv,&envcnt);
  int i,size=0;
  for(i=0;i<envcnt;i++){
    size+=strlen(penv[i].name)+strlen(penv[i].value)+2;
  } 
  *(uint32_t *)(uwmem->bytes + environ_count)=envcnt;
  *(uint32_t *)(uwmem->bytes + environ_buf_size)=size;
  sp = fp;
  pwart_rstack_put_i32(&sp, 0);
}
static void wasm__fd_advise(void *fp) { 
   void *sp = fp;
  _wargi32(fd) _wargi64(offset) _wargi64(len) _wargi32(advice)
      uvwasi_errno_t err = uvwasi_fd_advise(&uvwc, fd, offset, len, advice);
  sp = fp;
  pwart_rstack_put_i32(&sp, err);
}
static void wasm__fd_allocate(void *fp) { 
   void *sp = fp;
  _wargi32(fd) _wargi64(offset) _wargi64(len) uvwasi_errno_t err =
      uvwasi_fd_allocate(&uvwc, fd, offset, len);
  sp = fp;
  pwart_rstack_put_i32(&sp, err);
}
static void wasm__fd_close(void *fp) { 
   void *sp = fp;
  _wargi32(fd) uvwasi_errno_t err = uvwasi_fd_close(&uvwc, fd);
  sp = fp;
  pwart_rstack_put_i32(&sp, err);
}
static void wasm__fd_datasync(void *fp) { 
   void *sp = fp;
  _wargi32(fd) uvwasi_errno_t err = uvwasi_fd_datasync(&uvwc, fd);
  sp = fp;
  pwart_rstack_put_i32(&sp, err);
}
static void wasm__fd_fdstat_get(void *fp) { 
   void *sp = fp;
  _wargi32(fd) _wargi32(buf) uvwasi_errno_t err =
      uvwasi_fd_fdstat_get(&uvwc, fd, (uvwasi_fdstat_t *)(uwmem->bytes + buf));
  sp = fp;
  pwart_rstack_put_i32(&sp, err);
}
static void wasm__fd_fdstat_set_flags(void *fp) { 
   void *sp = fp;
  _wargi32(fd) _wargi32(flags) uvwasi_errno_t err =
      uvwasi_fd_fdstat_set_flags(&uvwc, fd, flags);
  sp = fp;
  pwart_rstack_put_i32(&sp, err);
}
static void wasm__fd_fdstat_set_rights(void *fp) { 
   void *sp = fp;
  _wargi32(fd) _wargi64(fs_rights_base) _wargi64(fs_rights_inheriting)
      uvwasi_errno_t err = uvwasi_fd_fdstat_set_rights(
          &uvwc, fd, fs_rights_base, fs_rights_inheriting);
  sp = fp;
  pwart_rstack_put_i32(&sp, err);
}
static void wasm__fd_filestat_get(void *fp) { 
   void *sp = fp;
  _wargi32(fd) _wargi32(buf) uvwasi_errno_t err =
      uvwasi_fd_filestat_get(&uvwc, fd, (uvwasi_filestat_t *)(uwmem->bytes + buf));
  sp = fp;
  pwart_rstack_put_i32(&sp, err);
}
static void wasm__fd_filestat_set_size(void *fp) { 
   void *sp = fp;
  _wargi32(fd) _wargi64(st_size) uvwasi_errno_t err =
      uvwasi_fd_filestat_set_size(&uvwc, fd, st_size);
  sp = fp;
  pwart_rstack_put_i32(&sp, err);
}
static void wasm__fd_filestat_set_times(void *fp) { 
   void *sp = fp;
  _wargi32(fd) _wargi64(st_atim) _wargi64(st_mtim) _wargi32(fst_flags)
      uvwasi_errno_t err =
          uvwasi_fd_filestat_set_times(&uvwc, fd, st_atim, st_mtim, fst_flags);
  sp = fp;
  pwart_rstack_put_i32(&sp, err);
}
static void wasm__fd_pread(void *fp) { 
   void *sp = fp;
  _wargi32(fd) _wargi32(iovs) _wargi32(iovs_len) _wargi64(offset)
  _wargi32(nread)
  uvwasi_iovec_t *iovs2=sp;
  uint32_t i;
  for(i=0;i<iovs_len;i++){
    iovs2[i].buf=uwmem->bytes+*(uint32_t *)(uwmem->bytes + iovs + 8*i);
    iovs2[i].buf_len=*(uint32_t *)(uwmem->bytes + iovs + 8*i+4);
  }
  uvwasi_errno_t err =
      uvwasi_fd_pread(&uvwc, fd, iovs2, iovs_len, offset,
                      (uvwasi_size_t *)(uwmem->bytes + nread));
  sp = fp;
  pwart_rstack_put_i32(&sp, err);
}
static void wasm__fd_prestat_get(void *fp) { 
   void *sp = fp;
  _wargi32(fd) _wargi32(buf) uvwasi_errno_t err =
      uvwasi_fd_prestat_get(&uvwc, fd, uwmem->bytes + buf);
  sp = fp;
  pwart_rstack_put_i32(&sp, err);
}
static void wasm__fd_prestat_dir_name(void *fp) { 
   void *sp = fp;
  _wargi32(fd) _wargi32(path) _wargi32(path_len) uvwasi_errno_t err =
      uvwasi_fd_prestat_dir_name(&uvwc, fd, uwmem->bytes + path, path_len);
  sp = fp;
  pwart_rstack_put_i32(&sp, err);
}
static void wasm__fd_pwrite(void *fp) { 
   void *sp = fp;
  _wargi32(fd) _wargi32(iovs) _wargi32(iovs_len) _wargi64(offset)
  _wargi32(nwritten)
  uvwasi_ciovec_t *iovs2=sp;
  uint32_t i;
  for(i=0;i<iovs_len;i++){
    iovs2[i].buf=uwmem->bytes+*(uint32_t *)(uwmem->bytes + iovs + 8*i);
    iovs2[i].buf_len=*(uint32_t *)(uwmem->bytes + iovs + 8*i+4);
  }
  uvwasi_errno_t err =
      uvwasi_fd_pwrite(&uvwc, fd, iovs2, iovs_len, offset,
                        uwmem->bytes + nwritten);
  sp = fp;
  pwart_rstack_put_i32(&sp, err);
}
static void wasm__fd_read(void *fp) { 
   void *sp = fp;
  _wargi32(fd) _wargi32(iovs) _wargi32(iovs_len) _wargi32(nread)
  uvwasi_iovec_t *iovs2=sp;
  uint32_t i;
  for(i=0;i<iovs_len;i++){
    iovs2[i].buf=uwmem->bytes+*(uint32_t *)(uwmem->bytes + iovs + 8*i);
    iovs2[i].buf_len=*(uint32_t *)(uwmem->bytes + iovs + 8*i+4);
  }
  uvwasi_errno_t err = uvwasi_fd_read(&uvwc, fd, iovs2,
                                      iovs_len, (uvwasi_size_t *)(uwmem->bytes + nread));
  sp = fp;
  pwart_rstack_put_i32(&sp, err);
}
static void wasm__fd_readdir(void *fp) { 
   void *sp = fp;
  _wargi32(fd) _wargi32(buf) _wargi32(buf_len) _wargi64(cookie)
      _wargi32(bufused) uvwasi_errno_t err =
          uvwasi_fd_readdir(&uvwc, fd, uwmem->bytes + buf, buf_len, cookie,
                            uwmem->bytes + bufused);
  sp = fp;
  pwart_rstack_put_i32(&sp, err);
}
static void wasm__fd_renumber(void *fp) { 
   void *sp = fp;
  _wargi32(from) _wargi32(to) uvwasi_errno_t err =
      uvwasi_fd_renumber(&uvwc, from, to);
  sp = fp;
  pwart_rstack_put_i32(&sp, err);
}
static void wasm__fd_seek(void *fp) { 
   void *sp = fp;
  _wargi32(fd) _wargi64(offset) _wargi32(whence) _wargi32(newoffset)
      uvwasi_errno_t err =
          uvwasi_fd_seek(&uvwc, fd, offset, whence, uwmem->bytes + newoffset);
  sp = fp;
  pwart_rstack_put_i32(&sp, err);
}
static void wasm__fd_sync(void *fp) { 
   void *sp = fp;
  _wargi32(fd) uvwasi_errno_t err = uvwasi_fd_sync(&uvwc, fd);
  sp = fp;
  pwart_rstack_put_i32(&sp, err);
}
static void wasm__fd_tell(void *fp) { 
   void *sp = fp;
  _wargi32(fd) _wargi32(offset) uvwasi_errno_t err =
      uvwasi_fd_tell(&uvwc, fd, uwmem->bytes + offset);
  sp = fp;
  pwart_rstack_put_i32(&sp, err);
}
static void wasm__fd_write(void *fp) { 
   void *sp = fp;
  _wargi32(fd) _wargi32(iovs) _wargi32(iovs_len) _wargi32(nwritten)
  uvwasi_ciovec_t *iovs2=sp;
  uint32_t i;
  for(i=0;i<iovs_len;i++){
    iovs2[i].buf=uwmem->bytes+*(uint32_t *)(uwmem->bytes + iovs + 8*i);
    iovs2[i].buf_len=*(uint32_t *)(uwmem->bytes + iovs + 8*i+4);
  }
  uvwasi_errno_t err = uvwasi_fd_write(&uvwc, fd, iovs2,
                                        iovs_len, uwmem->bytes + nwritten);
  sp = fp;
  pwart_rstack_put_i32(&sp, err);
}
static void wasm__path_create_directory(void *fp) { 
   void *sp = fp;
  _wargi32(fd) _wargi32(path) _wargi32(path_len) uvwasi_errno_t err =
      uvwasi_path_create_directory(&uvwc, fd, uwmem->bytes + path, path_len);
  sp = fp;
  pwart_rstack_put_i32(&sp, err);
}
static void wasm__path_filestat_get(void *fp) { 
   void *sp = fp;
  _wargi32(fd) _wargi32(flags) _wargi32(path) _wargi32(path_len) _wargi32(buf)
      uvwasi_errno_t err = uvwasi_path_filestat_get(
          &uvwc, fd, flags, uwmem->bytes + path, path_len, uwmem->bytes + buf);
  sp = fp;
  pwart_rstack_put_i32(&sp, err);
}
static void wasm__path_filestat_set_times(void *fp) { 
   void *sp = fp;
  _wargi32(fd) _wargi32(flags) _wargi32(path) _wargi32(path_len) _wargi64(
      st_atim) _wargi64(st_mtim) _wargi32(fst_flags) uvwasi_errno_t err =
      uvwasi_path_filestat_set_times(&uvwc, fd, flags, uwmem->bytes + path,
                                     path_len, st_atim, st_mtim, fst_flags);
  sp = fp;
  pwart_rstack_put_i32(&sp, err);
}
static void wasm__path_link(void *fp) { 
   void *sp = fp;
  _wargi32(old_fd) _wargi32(old_flags) _wargi32(old_path) _wargi32(old_path_len)
      _wargi32(new_fd) _wargi32(new_path) _wargi32(new_path_len)
          uvwasi_errno_t err =
              uvwasi_path_link(&uvwc, old_fd, old_flags,
                               uwmem->bytes + old_path, old_path_len, new_fd,
                               uwmem->bytes + new_path, new_path_len);
  sp = fp;
  pwart_rstack_put_i32(&sp, err);
}
static void wasm__path_open(void *fp) { 
   void *sp = fp;
  _wargi32(dirfd) _wargi32(dirflags) _wargi32(path) _wargi32(path_len)
      _wargi32(o_flags) _wargi64(fs_rights_base) _wargi64(fs_rights_inheriting)
          _wargi32(fs_flags) _wargi32(fd) uvwasi_errno_t err =
              uvwasi_path_open(&uvwc, dirfd, dirflags, uwmem->bytes + path,
                               path_len, o_flags, fs_rights_base,
                               fs_rights_inheriting, fs_flags,
                               uwmem->bytes + fd);
  sp = fp;
  pwart_rstack_put_i32(&sp, err);
}
static void wasm__path_readlink(void *fp) { 
   void *sp = fp;
  _wargi32(fd) _wargi32(path) _wargi32(path_len) _wargi32(buf) _wargi32(buf_len)
      _wargi32(bufused) uvwasi_errno_t err =
          uvwasi_path_readlink(&uvwc, fd, uwmem->bytes + path, path_len,
                               uwmem->bytes + buf, buf_len,
                               uwmem->bytes + bufused);
  sp = fp;
  pwart_rstack_put_i32(&sp, err);
}
static void wasm__path_remove_directory(void *fp) { 
   void *sp = fp;
  _wargi32(fd) _wargi32(path) _wargi32(path_len) uvwasi_errno_t err =
      uvwasi_path_remove_directory(&uvwc, fd, uwmem->bytes + path, path_len);
  sp = fp;
  pwart_rstack_put_i32(&sp, err);
}
static void wasm__path_rename(void *fp) { 
   void *sp = fp;
  _wargi32(old_fd) _wargi32(old_path) _wargi32(old_path_len) _wargi32(new_fd)
      _wargi32(new_path) _wargi32(new_path_len) uvwasi_errno_t err =
          uvwasi_path_rename(&uvwc, old_fd, uwmem->bytes + old_path,
                             old_path_len, new_fd, uwmem->bytes + new_path,
                             new_path_len);
  sp = fp;
  pwart_rstack_put_i32(&sp, err);
}
static void wasm__path_symlink(void *fp) { 
   void *sp = fp;
  _wargi32(old_path) _wargi32(old_path_len) _wargi32(fd) _wargi32(new_path)
      _wargi32(new_path_len) uvwasi_errno_t err =
          uvwasi_path_symlink(&uvwc, uwmem->bytes + old_path, old_path_len, fd,
                              uwmem->bytes + new_path, new_path_len);
  sp = fp;
  pwart_rstack_put_i32(&sp, err);
}
static void wasm__path_unlink_file(void *fp) { 
   void *sp = fp;
  _wargi32(fd) _wargi32(path) _wargi32(path_len) uvwasi_errno_t err =
      uvwasi_path_unlink_file(&uvwc, fd, uwmem->bytes + path, path_len);
  sp = fp;
  pwart_rstack_put_i32(&sp, err);
}
static void wasm__poll_oneoff(void *fp) { 
   void *sp = fp;
  _wargi32(in) _wargi32(out) _wargi32(nsubscriptions) _wargi32(nevents)
      uvwasi_errno_t err =
          uvwasi_poll_oneoff(&uvwc, uwmem->bytes + in, uwmem->bytes + out,
                             nsubscriptions, uwmem->bytes + nevents);
  sp = fp;
  pwart_rstack_put_i32(&sp, err);
}
static void wasm__proc_exit(void *fp) { 
   void *sp = fp;
  _wargi32(rval) uvwasi_errno_t err = uvwasi_proc_exit(&uvwc, rval);
  sp = fp;
  pwart_rstack_put_i32(&sp, err);
}
static void wasm__proc_raise(void *fp) { 
   void *sp = fp;
  _wargi32(sig) uvwasi_errno_t err = uvwasi_proc_raise(&uvwc, sig);
  sp = fp;
  pwart_rstack_put_i32(&sp, err);
}
static void wasm__random_get(void *fp) { 
   void *sp = fp;
  _wargi32(buf) _wargi32(buf_len) uvwasi_errno_t err =
      uvwasi_random_get(&uvwc, uwmem->bytes + buf, buf_len);
  sp = fp;
  pwart_rstack_put_i32(&sp, err);
}
static void wasm__sched_yield(void *fp) { 
   void *sp = fp;
  uvwasi_errno_t err = uvwasi_sched_yield(&uvwc);
  sp = fp;
  pwart_rstack_put_i32(&sp, err);
}
static void wasm__sock_accept(void *fp) { 
   void *sp = fp;
  _wargi32(sock) _wargi32(flags) _wargi32(fd) uvwasi_errno_t err =
      uvwasi_sock_accept(&uvwc, sock, flags, uwmem->bytes + fd);
  sp = fp;
  pwart_rstack_put_i32(&sp, err);
}
static void wasm__sock_recv(void *fp) { 
   void *sp = fp;
  _wargi32(sock) _wargi32(ri_data) _wargi32(ri_data_len) _wargi32(ri_flags)
      _wargi32(ro_datalen) _wargi32(ro_flags) uvwasi_errno_t err =
          uvwasi_sock_recv(&uvwc, sock, uwmem->bytes + ri_data, ri_data_len,
                           ri_flags, uwmem->bytes + ro_datalen,
                           uwmem->bytes + ro_flags);
  sp = fp;
  pwart_rstack_put_i32(&sp, err);
}
static void wasm__sock_send(void *fp) { 
   void *sp = fp;
  _wargi32(sock) _wargi32(si_data) _wargi32(si_data_len) _wargi32(si_flags)
      _wargi32(so_datalen) uvwasi_errno_t err =
          uvwasi_sock_send(&uvwc, sock, uwmem->bytes + si_data, si_data_len,
                           si_flags, uwmem->bytes + so_datalen);
  sp = fp;
  pwart_rstack_put_i32(&sp, err);
}
static void wasm__sock_shutdown(void *fp) { 
   void *sp = fp;
  _wargi32(sock) _wargi32(how) uvwasi_errno_t err =
      uvwasi_sock_shutdown(&uvwc, sock, how);
  sp = fp;
  pwart_rstack_put_i32(&sp, err);
}

extern void pwart_wasi_module_set_wasimemory(struct pwart_wasm_memory *m) {
  uwmem = m;
}

static uint32_t wasiargc = 0;
static char **wasiargv = NULL;

extern void pwart_wasi_module_set_wasiargs(uint32_t argc, char **argv) {
  wasiargc = argc;
  wasiargv = argv;
}




extern char *pwart_wasi_module_init(){
  uvwasi_options_t init_options;
  uvwasi_errno_t err;
  uvwasi_options_init(&init_options);
  if(uvwcinited){
    uvwasi_destroy(&uvwc);
    uvwcinited=0;
  }
  /* Setup the initialization options. */
  init_options.in = fileno(stdin);
  init_options.out = fileno(stdout);
  init_options.err = fileno(stderr);
  init_options.fd_table_size = 3;
  /* Android failed with environ, Use uv_os_environ? */
  /* init_options.envp = (const char **)environ; */
  init_options.envp=NULL;
  init_options.preopenc = 1;
  init_options.preopens = wa_malloc(sizeof(uvwasi_preopen_t));
  init_options.preopens->mapped_path="/";
  init_options.preopens->real_path=".";
  init_options.allocator = NULL;
  init_options.argc = wasiargc;
  init_options.argv = wasiargv;

  /* Initialize the sandbox. */
  err = uvwasi_init(&uvwc, &init_options);
  if(err)return "uvwasi_init failed.";
  uvwcinited=1;
  return NULL;
}

static char *wasi_funcname_list[]={
  "args_get","args_sizes_get","clock_res_get","clock_time_get","environ_get",
  "environ_sizes_get","fd_advise","fd_allocate","fd_close","fd_datasync",
  "fd_fdstat_get","fd_fdstat_set_flags","fd_fdstat_set_rights","fd_filestat_get","fd_filestat_set_size",
  "fd_filestat_set_times","fd_pread","fd_prestat_get","fd_prestat_dir_name","fd_pwrite",
  "fd_read","fd_readdir","fd_renumber","fd_seek","fd_sync",
  "fd_tell","fd_write","path_create_directory","path_filestat_get","path_filestat_set_times",
  "path_link","path_open","path_readlink","path_remove_directory","path_rename",
  "path_symlink","path_unlink_file","poll_oneoff","proc_exit","proc_raise",
  "random_get","sched_yield","sock_accept","sock_recv","sock_send",
  "sock_shutdown"
};

static pwart_host_function_c wasi_funcsymbols_list[]={
  &wasm__args_get,&wasm__args_sizes_get,&wasm__clock_res_get,&wasm__clock_time_get,&wasm__environ_get,
  &wasm__environ_sizes_get,&wasm__fd_advise,&wasm__fd_allocate,&wasm__fd_close,&wasm__fd_datasync,
  &wasm__fd_fdstat_get,&wasm__fd_fdstat_set_flags,&wasm__fd_fdstat_set_rights,&wasm__fd_filestat_get,&wasm__fd_filestat_set_size,
  &wasm__fd_filestat_set_times,&wasm__fd_pread,&wasm__fd_prestat_get,&wasm__fd_prestat_dir_name,&wasm__fd_pwrite,
  &wasm__fd_read,&wasm__fd_readdir,&wasm__fd_renumber,&wasm__fd_seek,&wasm__fd_sync,
  &wasm__fd_tell,&wasm__fd_write,&wasm__path_create_directory,&wasm__path_filestat_get,&wasm__path_filestat_set_times,
  &wasm__path_link,&wasm__path_open,&wasm__path_readlink,&wasm__path_remove_directory,&wasm__path_rename,
  &wasm__path_symlink,&wasm__path_unlink_file,&wasm__poll_oneoff,&wasm__proc_exit,&wasm__proc_raise,
  &wasm__random_get,&wasm__sched_yield,&wasm__sock_accept,&wasm__sock_recv,&wasm__sock_send,
  &wasm__sock_shutdown
};

extern struct pwart_host_module *pwart_wasi_module_new() {
  return pwart_namespace_new_host_module(wasi_funcname_list,wasi_funcsymbols_list,46);
}

extern char *pwart_wasi_module_delete(struct pwart_host_module *mod) {
  wa_free(mod);
  if(uvwcinited){
    uvwasi_destroy(&uvwc);
    uvwcinited=0;
  }
  return NULL;
}

#endif