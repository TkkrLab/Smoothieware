/*
      This file is part of Smoothie (http://smoothieware.org/). The motion control part is heavily based on Grbl (https://github.com/simen/grbl).
      Smoothie is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
      Smoothie is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
      You should have received a copy of the GNU General Public License along with Smoothie. If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef ENDSTOPS_MODULE_H
#define ENDSTOPS_MODULE_H

#include "libs/Module.h"
#include "libs/Pin.h"

#include <bitset>

class StepperMotor;
class Gcode;

class Endstops : public Module{
    public:
        Endstops();
        void on_module_loaded();
        void on_gcode_received(void* argument);
        void acceleration_tick(void);

    private:
        void load_config();
        void home(char axes_to_move);
        void do_homing_cartesian(char axes_to_move);
        void do_homing_corexy(char axes_to_move);
        bool wait_for_homed(char axes_to_move);
        bool wait_for_homed_corexy(int axis);
        void corexy_home(int home_axis, bool dirx, bool diry, float fast_rate, float slow_rate, unsigned int retract_steps);
        void back_off_home(char axes_to_move);
        void move_to_origin(char);
        void on_get_public_data(void* argument);
        void on_set_public_data(void* argument);
        void on_idle(void *argument);
        bool debounced_get(int pin);
        void process_home_command(Gcode* gcode);
        void set_homing_offset(Gcode* gcode);

        float homing_position[3];
        float home_offset[3];
        uint8_t homing_order;
        std::bitset<3> home_direction;
        std::bitset<3> limit_enable;
        float saved_position[3]{0}; // save G28 (in grbl mode)

        unsigned int  debounce_count;
        float  retract_mm[3];
        float  trim_mm[3];
        float  fast_rates[3];
        float  slow_rates[3];
        Pin    pins[7];
        volatile float feed_rate[3];
        struct {
            bool is_corexy:1;
            bool is_delta:1;
            bool is_rdelta:1;
            bool is_scara:1;
            bool move_to_origin_after_home:1;
            uint8_t bounce_cnt:4;
            volatile char status:3;
        };
        float  soft_max[3];
};

#endif
