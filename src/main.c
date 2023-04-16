/* PicoRuby */
#include <picorbc.h>
#include <picogem_init.c>

/* General */
#include <stdio.h>
#include <stdlib.h>

/* tinyusb */
#include <tusb.h>

/* mrbc_class */
#include "../include/gpio.h"
#include "../include/usb_descriptors.h"
#include "../include/msc_disk.h"
#include "../include/version.h"

/* ruby */
/* ext */
//#include "../build/mrb/object-ext.c"
/* tasks */
#include "../build/mrb/usb_task.c"

#ifdef PRK_NO_MSC
#include <keymap.c>
#endif

#define MEMORY_SIZE (1024*128)

static uint8_t memory_pool[MEMORY_SIZE];

/* extern in mruby-pico-compiler/include/debug.h */
int loglevel = LOGLEVEL_WARN;

//--------------------------------------------------------------------+
// Device Descriptors
//--------------------------------------------------------------------+
tusb_desc_device_t desc_device =
{
  .bLength            = sizeof(tusb_desc_device_t),
  .bDescriptorType    = TUSB_DESC_DEVICE,
  .bcdUSB             = 0x0200,
  // Use Interface Association Descriptor (IAD) for CDC
  // As required by USB Specs IAD's subclass must be common class (2) and protocol must be IAD (1)
  .bDeviceClass       = TUSB_CLASS_MISC,
  .bDeviceSubClass    = MISC_SUBCLASS_COMMON,
  .bDeviceProtocol    = MISC_PROTOCOL_IAD,
  .bMaxPacketSize0    = CFG_TUD_ENDPOINT0_SIZE,
  /*
   * VID and PID from USB-IDs-for-free.txt
   * https://github.com/obdev/v-usb/blob/releases/20121206/usbdrv/USB-IDs-for-free.txt#L128
   */
  .idVendor           = 0x16c0,
  .idProduct          = 0x27db,
  .bcdDevice          = 0x0100,
  .iManufacturer      = 0x01,
  .iProduct           = 0x02,
  .iSerialNumber      = 0x03,
  .bNumConfigurations = 0x01
};

//--------------------------------------------------------------------+
// String Descriptors
//--------------------------------------------------------------------+
#include "../include/version.h"
char const *string_desc_arr[STRING_DESC_ARR_SIZE] =
{
  (const char[]) { 0x09, 0x04 }, // 0: is supported language is English (0x0409)
  "PRK Firmware developers",     // 1: Manufacturer
  "Default VID/PID",             // 2: Product
  PRK_SERIAL,                    // 3: Serial
  "PRK CDC",                     // 4: CDC Interface
  "PRK MSC",                     // 5: MSC Interface
};

/* class PicoRubyVM */

static void
quick_print_alloc_stats()
{
  struct MRBC_ALLOC_STATISTICS mem;
  mrbc_alloc_statistics(&mem);
  console_printf("\nSTATS %d/%d\n", mem.used, mem.total);
  //tud_task();
}

static void
c_alloc_stats(mrb_vm *vm, mrb_value *v, int argc)
{
  struct MRBC_ALLOC_STATISTICS mem;
  mrbc_value ret = mrbc_hash_new(vm, 4);
  mrbc_alloc_statistics(&mem);
  mrbc_hash_set(&ret, &mrbc_symbol_value(mrbc_str_to_symid("TOTAL")),
                &mrbc_integer_value(mem.total));
  mrbc_hash_set(&ret, &mrbc_symbol_value(mrbc_str_to_symid("USED")),
                &mrbc_integer_value(mem.used));
  mrbc_hash_set(&ret, &mrbc_symbol_value(mrbc_str_to_symid("FREE")),
                &mrbc_integer_value(mem.free));
  mrbc_hash_set(&ret, &mrbc_symbol_value(mrbc_str_to_symid("FRAGMENTATION")),
                &mrbc_integer_value(mem.fragmentation));
  SET_RETURN(ret);
}

/* class Machine */
/*
static void
c_board_millis(mrb_vm *vm, mrb_value *v, int argc)
{
  SET_INT_RETURN(board_millis());
}

static void
c_srand(mrb_vm *vm, mrb_value *v, int argc)
{
  srand(GET_INT_ARG(1));
}

static void
c_rand(mrb_vm *vm, mrb_value *v, int argc)
{
  SET_INT_RETURN(rand());
}
*/
int autoreload_state = AUTORELOAD_READY; /* from msc_disk.h */

#ifndef PRK_NO_MSC

#ifndef NODE_BOX_SIZE
#define NODE_BOX_SIZE 50
#endif

#define KEYMAP_PREFIX        "begin\n"
#define KEYMAP_PREFIX_SIZE   (sizeof(KEYMAP_PREFIX) - 1)
#define KEYMAP_POSTFIX       "\nrescue => e\nputs e.class, e.message, 'Task stopped!'\nend"
#define KEYMAP_POSTFIX_SIZE  (sizeof(KEYMAP_POSTFIX))
#define SUSPEND_TASK         "while true;sleep 5;puts 'Please make keymap.rb in PRKFirmware drive';end;"
#define MAX_KEYMAP_SIZE      (1024 * 10)

static void
reset_vm(mrbc_vm *vm)
{
  vm->cur_irep        = vm->top_irep;
  vm->inst            = vm->cur_irep->inst;
  vm->cur_regs        = vm->regs;
  vm->target_class    = mrbc_class_object;
  vm->callinfo_tail   = NULL;
  vm->ret_blk         = NULL;
  vm->exception       = mrbc_nil_value();
  vm->flag_preemption = 0;
  vm->flag_stop       = 0;
}

#ifndef PRK_NO_MSC
mrbc_tcb *tcb_keymap;

static uint32_t buffer = 0;
static int      buffer_index = 0;

static void
c_Keyboard_resume_keymap(mrbc_vm *vm, mrbc_value *v, int argc)
{
  mrbc_resume_task(tcb_keymap);
}

static void
c_Keyboard_suspend_keymap(mrbc_vm *vm, mrbc_value *v, int argc)
{
  mrbc_suspend_task(tcb_keymap);
}

/*
 * TODO: The current implemantation is in prk_firmware/src/main.c
 *       Move that to this gem along with taking advantage of File and Dir class
 */
mrbc_tcb* create_keymap_task(mrbc_tcb *tcb);

static void
c_Keyboard_reload_keymap(mrbc_vm *vm, mrbc_value *v, int argc)
{
  tcb_keymap = create_keymap_task(tcb_keymap);
}

static void c_autoreload_ready_q(mrb_vm *vm, mrb_value *v, int argc)
{
  if (autoreload_state == AUTORELOAD_READY) {
    SET_TRUE_RETURN();
  } else {
    SET_FALSE_RETURN();
  }
}
#endif /* PRK_NO_MSC */


mrbc_tcb*
create_keymap_task(mrbc_tcb *tcb)
{
  hal_disable_irq();
  DirEnt entry;
  StreamInterface *si;
  ParserState *p = Compiler_parseInitState(NULL, NODE_BOX_SIZE);
  msc_findDirEnt("KEYMAP  RB ", &entry);
  static uint8_t *keymap_rb = NULL;
  if (keymap_rb) picorbc_free(keymap_rb);
  if (entry.Name[0] != '\0') {
    //RotaryEncoder_reset();
    uint32_t fileSize = entry.FileSize;
    console_printf("keymap.rb size: %u\n", fileSize);
    //tud_task();
    if (fileSize < MAX_KEYMAP_SIZE) {
      keymap_rb = picorbc_alloc(KEYMAP_PREFIX_SIZE + fileSize + KEYMAP_POSTFIX_SIZE);
      memcpy(keymap_rb, KEYMAP_PREFIX, KEYMAP_PREFIX_SIZE);
      msc_loadFile(keymap_rb + KEYMAP_PREFIX_SIZE, &entry);
      memcpy(keymap_rb + KEYMAP_PREFIX_SIZE + fileSize, KEYMAP_POSTFIX, KEYMAP_POSTFIX_SIZE);
      si = StreamInterface_new(NULL, (char *)keymap_rb, STREAM_TYPE_MEMORY);
    } else {
      console_printf("Must be less than %d bytes!\n", MAX_KEYMAP_SIZE);
      //tud_task();
      si = StreamInterface_new(NULL, SUSPEND_TASK, STREAM_TYPE_MEMORY);
    }
  } else {
    console_printf("No keymap.rb found!\n");
    //tud_task();
    si = StreamInterface_new(NULL, SUSPEND_TASK, STREAM_TYPE_MEMORY);
  }
  uint8_t *vm_code = NULL;
  if (Compiler_compile(p, si, NULL)) {
    vm_code = p->scope->vm_code;
    p->scope->vm_code = NULL;
  } else {
    console_printf("Compiling keymap.rb failed!\n");
    //tud_task();
    Compiler_parserStateFree(p);
    StreamInterface_free(si);
    p = Compiler_parseInitState(NULL, NODE_BOX_SIZE);
    si = StreamInterface_new(NULL, SUSPEND_TASK, STREAM_TYPE_MEMORY);
    Compiler_compile(p, si, NULL);
  }
  quick_print_alloc_stats();
  Compiler_parserStateFree(p);
  StreamInterface_free(si);
  if (tcb == NULL) {
    tcb = mrbc_create_task(vm_code, 0);
  } else {
    mrbc_suspend_task(tcb);
    if(mrbc_load_mrb(&tcb->vm, vm_code) != 0) {
      console_printf("Loading keymap.mrb failed!\n");
      //tud_task();
    } else {
      reset_vm(&tcb->vm);
      mrbc_resume_task(tcb);
    }
  }
  autoreload_state = AUTORELOAD_WAIT;
  hal_enable_irq();
  return tcb;
}

#endif /* PRK_NO_MSC */

static void
prk_init_picoruby(void)
{
  /* CONST */
  mrbc_sym sym_id = mrbc_str_to_symid("SIZEOF_POINTER");
  mrbc_set_const(sym_id, &mrbc_integer_value(PICORBC_PTR_SIZE));
  mrbc_vm *vm = mrbc_vm_open(NULL);
  sym_id = mrbc_str_to_symid("PRK_DESCRIPTION");
  mrbc_value prk_desc = mrbc_string_new_cstr(vm, PRK_DESCRIPTION);
  mrbc_set_const(sym_id, &prk_desc);
  mrbc_raw_free(vm);
  /* class Object */
  //picoruby_load_model(object_ext);
  picoruby_init_require();
  /* class Machine */
  mrbc_class *mrbc_class_Machine = mrbc_define_class(0, "Machine", mrbc_class_object);
  /* class PicoRubyVM */
  mrbc_class *mrbc_class_PicoRubyVM = mrbc_define_class(0, "PicoRubyVM", mrbc_class_object);
  /* GPIO */
  prk_init_gpio();
  prk_init_usb();
#ifndef PRK_NO_MSC
  mrbc_class *mrbc_class_Keyboard = mrbc_define_class(0, "Keyboard", mrbc_class_object);

  mrbc_define_method(0, mrbc_class_Keyboard, "reload_keymap",  c_Keyboard_reload_keymap);
  mrbc_define_method(0, mrbc_class_Keyboard, "suspend_keymap", c_Keyboard_suspend_keymap);
  mrbc_define_method(0, mrbc_class_Keyboard, "resume_keymap",  c_Keyboard_resume_keymap);
  mrbc_define_method(0, mrbc_class_Keyboard, "autoreload_ready?", c_autoreload_ready_q);
#endif /* PRK_NO_MSC */
}


int
main(void)
{
//#ifndef PRK_NO_MSC
  board_init();
  tusb_init();
//#endif
  //
  /* PicoRuby */
  mrbc_init(memory_pool, MEMORY_SIZE);
  prk_init_picoruby();
  /* Tasks */
  mrbc_create_task(usb_task, 0);
#ifdef PRK_NO_MSC
  mrbc_create_task(keymap, 0);
#else
  prk_msc_init();
#endif
  mrbc_run();
  while(1) { tud_task(); }
  return 0;
}
