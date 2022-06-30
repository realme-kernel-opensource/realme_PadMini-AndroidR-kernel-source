#ifndef CTS_OEM_H
#define CTS_OEM_H

struct chipone_ts_data;

#define CTS_PROC_TOUCHPANEL_FOLDER "touchpanel"
extern struct proc_dir_entry *CTS_proc_touchpanel_dir;
extern int cts_oem_init(struct chipone_ts_data *cts_data);
extern int cts_oem_deinit(struct chipone_ts_data *cts_data);

#endif /* CTS_VENDOR_H */

