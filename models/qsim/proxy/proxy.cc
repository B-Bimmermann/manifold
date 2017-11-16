#include "proxy.h"
#include "qsim.h"
#include "qsim-load.h"

using namespace std;
using namespace Qsim;
using namespace manifold;
using namespace manifold::kernel;
using namespace manifold::qsim_proxy;

qsim_proxy_t::qsim_proxy_t(char *StateName, char *AppName, uint64_t InterruptInterval) :
    qsim_osd(NULL),
    interrupt_interval(InterruptInterval)
{
    cout << "Loading Qsim state file ..." << endl;
    qsim_osd = new Qsim::OSDomain(StateName);

    cout << "Loading application ..." << endl;
    load_file(*qsim_osd, AppName);

    cout << "Finished initializing Qsim" << endl;

    /* Set Qsim callback functions */
    /* Setup the callbacks after loading state file */
    qsim_osd->set_inst_cb(this, &qsim_proxy_t::inst_cb);
    qsim_osd->set_mem_cb(this, &qsim_proxy_t::mem_cb);
    qsim_osd->set_reg_cb(this, &qsim_proxy_t::reg_cb);

    /* to enable system callbacks */
    //qsim_osd->set_sys_cbs(true);

    buffer.reserve(QSIM_PROXY_QUEUE_SIZE);
    buffer.clear();
}

qsim_proxy_t::qsim_proxy_t(char *StateName, char *AppName, uint64_t InterruptInterval, char * Petalinux_project_folder_path, char * Qemu_shared_folder_path) :
    qsim_osd(NULL),
    interrupt_interval(InterruptInterval)
{
    cout << "Loading Qsim with ZCU102 ..." << endl;
    qsim_osd = new Qsim::OSDomain(4, Petalinux_project_folder_path, "xilinx_zcu102",QSIM_HEADLESS, 4096,Qemu_shared_folder_path);

    cout << "Have fineshed Kernel start-up. Start application ..." << endl;
    load_file(*qsim_osd, AppName);

    cout << "Finished initializing Qsim" << endl;

    /* Set Qsim callback functions */
    /* Setup the callbacks after loading state file */
    qsim_osd->set_inst_cb(this, &qsim_proxy_t::inst_cb);
    qsim_osd->set_mem_cb(this, &qsim_proxy_t::mem_cb);
    qsim_osd->set_reg_cb(this, &qsim_proxy_t::reg_cb);

    /* to enable system callbacks */
    //qsim_osd->set_sys_cbs(true);

    buffer.reserve(QSIM_PROXY_QUEUE_SIZE);
    buffer.clear();
}


qsim_proxy_t::~qsim_proxy_t()
{
    cout << "Terminating Qsim" << endl;
    delete qsim_osd;
}


void qsim_proxy_t::tick()
{
    uint64_t clock_cycle = m_clk->NowTicks();

    /* Invoke Qsim timer interrupt */
    if(clock_cycle % interrupt_interval == 0) {
        qsim_osd->timer_interrupt();
    }
}


void qsim_proxy_t::tock()
{
    /* Nothing to do */
}


void qsim_proxy_t::inst_cb(int core_id, uint64_t vaddr, uint64_t paddr, uint8_t len, const uint8_t *bytes, enum inst_type type)
{
#ifdef DEBUG_NEW_QSIM
    std::cerr << "( core " << std::dec << core_id << " ): INST" << " | v: 0x" << std::hex << vaddr <<" p: 0x" << std::hex << paddr << std::endl << std::flush;
#endif
    buffer.push_back(QueueItem(core_id, vaddr, paddr, len, bytes, type));
}


void qsim_proxy_t::mem_cb(int core_id, uint64_t vaddr, uint64_t paddr, uint8_t size, int type)
{
#ifdef DEBUG_NEW_QSIM
    std::cerr << "@ " << std::dec << manifold::kernel::Manifold::NowTicks() << " ( core " << std::dec << core_id << " ): MEM" << " | v: 0x" << std::hex << vaddr <<" p: 0x" << std::hex << paddr << (type == 0 ? " RD" : " WR") << std::endl << std::flush;
#endif
    buffer.push_back(QueueItem(core_id, vaddr, paddr, size, type));
}


void qsim_proxy_t::reg_cb(int core_id, int reg, uint8_t size, int type)
{
#ifdef DEBUG_NEW_QSIM
    std::cerr << "( core " << std::dec << core_id << " ): REG" << " | regid: " << std::dec << reg << " size: " << static_cast<uint16_t>(size) << (type == 0 ? " SRC" : " DST") << std::endl << std::flush;
#endif
    buffer.push_back(QueueItem(core_id, reg, size, type));
}
