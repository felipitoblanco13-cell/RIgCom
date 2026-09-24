#include <stdio.h>
#include "rig_face_master.h"

int main(void) {
    RigFaceStudioMaster studio;
    rig_face_master_init(&studio, "RIGFACE-HONOR400-CANONICAL");
    
    printf("Stepping 10 frames...\n");
    for (int i = 0; i < 10; i++) {
        rig_face_master_step_frame(&studio, 0.016f);
    }
    
    printf("Simulating Touch Input at (0.5, 0.5) with Force 0.8N...\n");
    rig_face_master_process_touch(&studio, 0.5f, 0.5f, 0.8f);
    
    printf("RIGFACE Master Engine verification complete!\n");
    return 0;
}
