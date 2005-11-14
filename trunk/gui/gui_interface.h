#ifndef __GUI_INTERFACE_H__
#define __GUI_INTERFACE_H__


#ifdef __cplusplus
//extern "C" {
#endif /* __cplusplus */


void gui_new_processor (unsigned int pic_id);
void gui_new_source (unsigned int pic_id);

void update_register(unsigned int reg_number);
void update_program_memory(GUI_Processor *gp, unsigned int reg_number);


#ifdef __cplusplus
//}
#endif /* __cplusplus */


#endif /* __GUI_INTERFACE_H__ */
