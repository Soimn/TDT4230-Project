// This file is a port of pcg 32 rxs m xs, the license for the original c
// code is included below.

/*
 * PCG Random Number Generation for C.
 *
 * Copyright 2014 Melissa O'Neill <oneill@pcg-random.org>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * For additional information about the PCG random number generation scheme,
 * including its license and other licensing options, visit
 *
 *     http://www.pcg-random.org
 */

#define PCG_DEFAULT_MULTIPLIER_32  747796405U
#define PCG_DEFAULT_INCREMENT_32   2891336453U

/*
struct pcg32_state
{
	uint state;
	uint increment;
};
*/

void
pcg32_seed(inout pcg32_state pcg_state, uint seed, uint increment)
{
    pcg_state.state = 0;
    pcg_state.increment = (increment << 1u) | 1u;

    pcg_state.state = pcg_state.state * PCG_DEFAULT_MULTIPLIER_32 + pcg_state.increment;
    pcg_state.state += seed;
    pcg_state.state = pcg_state.state * PCG_DEFAULT_MULTIPLIER_32 + pcg_state.increment;
}

uint
pcg32_next(inout pcg32_state pcg_state)
{
    uint old_state = pcg_state.state;
    pcg_state.state = pcg_state.state * PCG_DEFAULT_MULTIPLIER_32 + pcg_state.increment;

    uint word = ((old_state >> ((old_state >> 28u) + 4u)) ^ old_state) * 277803737u;
    return (word >> 22u) ^ word;
}
