-- =========================================================
-- MULTISTAGE SPY MFD
-- Diagnostic Monitor for SLS Configurations
-- =========================================================

local page = 1
local total_pages = 6
local dispw, disph = 0, 0
local cw, ch = 1, 1
local is_logging = false
local last_log_time = 0
local log_interval = 1.0
local csv_header_written = false

-- MFD Menu Definitions
local btn_label = {'PRV', 'NXT', 'LOG'}
local btn_menu = {
    {l1='Previous Page', sel='p'},
    {l1='Next Page', sel='n'},
    {l1='Log Page to File', sel='l'}
}

function setup(w, h)
    dispw = w
    disph = h
    cw, ch = 1, 1
end

function buttonlabel(bt)
    if bt < 3 then return btn_label[bt+1] end
    return nil
end

function buttonmenu()
    return btn_menu, 3
end

-- =========================================================
-- Auto Pilot Status
-- =========================================================
local function get_vinka_info(v)
    local ap_on = 0
    local config_state = 0
    local prog_code = 0
    local status = "PROG 00 - OFFLINE"

    -- READ THE PHANTOM DATA BUS
    local t_count = v:get_thrustercount()
    for i = 0, t_count - 1 do
        local th = v:get_thrusterhandle(i)
        if th then
            local pos = v:get_thrusterpos(th)
            -- Check if this is one of our Data Bus thrusters (Y is near 999.0)
            if pos.y > 998.0 and pos.y < 1000.0 then
                local val = v:get_thrustermax0(th)
                if pos.x > 0.5 and pos.x < 1.5 then ap_on = math.floor(val + 0.5)
                elseif pos.x > 1.5 and pos.x < 2.5 then config_state = math.floor(val + 0.5)
                elseif pos.x > 2.5 and pos.x < 3.5 then prog_code = math.floor(val + 0.5)
                end
            end
        end
    end

    if ap_on == 1 then
        if config_state == 0 then
            status = "PROG 01 - PRELAUNCH INIT"
        elseif config_state == 1 then

            -- THE APOLLO DSKY DECODER
            if prog_code == 0 then status = "PROG 00 - CMC IDLING"
            elseif prog_code == 1 then status = "PROG 01 - PRELAUNCH INIT"
            elseif prog_code == 4 then status = "PROG 04 - ENGINE IGNIT"
            elseif prog_code == 5 then status = "PROG 05 - TOWER JETTISON"
            elseif prog_code == 6 then status = "PROG 06 - FAIRING JET"
            elseif prog_code == 10 then status = "PROG 10 - TOWER CLEAR"
            elseif prog_code == 11 then status = "PROG 11 - ASCENT ROLL"
            elseif prog_code == 12 then status = "PROG 12 - ASCENT PITCH"
            elseif prog_code == 13 then status = "PROG 13 - ASCENT PITCH OV"
            elseif prog_code == 14 then status = "PROG 14 - GRAVITY TURN"
            elseif prog_code == 15 then status = "PROG 15 - PEG INSERTION"
            elseif prog_code == 16 then status = "PROG 16 - MECO"
            elseif prog_code == 20 then status = "PROG 20 - RENDEZVOUS"
            elseif prog_code == 30 then status = "PROG 30 - EXT DELTA-V"
            elseif prog_code == 40 then status = "PROG 40 - ORBIT BURN"
            elseif prog_code == 61 then status = "PROG 61 - ENTRY PREP"
            elseif prog_code == 62 then status = "PROG 62 - JETTISON"
            elseif prog_code == 66 then status = "PROG 66 - AERO GUIDANCE"
            elseif prog_code == 99 then status = "PROG 99 - VEHICLE DEST"
            else status = string.format("PROG %02d - ACTIVE", prog_code) end

        end
    else
        status = "PROG 00 - CMC IDLING"
    end
    return ap_on, prog_code, status
end

-- =========================================================
-- LOGGING FUNCTIONALITY (CSV Telemetry Dump)
-- =========================================================
local function log_to_file(text)
    local f = io.open("SLS_Telemetry.csv", "a") -- Changed to .csv!
    if f then
        f:write(text .. "\n")
        f:close()
    else
        oapi.write_log("SPY MFD ERROR: Could not open SLS_Telemetry.csv")
    end
end

local function log_telemetry_csv(v)
    -- 1. DUMP HEADER (Using Prog_Code instead of System_Status)
    if not csv_header_written then
        log_to_file("MET,Prog_Code,Altitude_m,Mass_kg,Pitch_deg,Thrust_MN,TWR")
        csv_header_written = true
    end

    -- 2. GATHER DATA
    local ap_on, prog_code, vinka_status = get_vinka_info(v)
    local met = oapi.get_simtime()
    local alt = v:get_altitude()
    local mass = v:get_mass()
    local pitch = v:get_pitch() * 57.2957
    
    -- Calculate Thrust and TWR
    local thrust_val = 0
    local main_throttle = v:get_thrustergrouplevel(0)
    if main_throttle ~= nil then
        local th_count = v:get_thrustercount()
        for i = 0, th_count - 1 do
            local th = v:get_thrusterhandle(i)
            if th then
                -- Ignore the Phantom Data Bus thrusters!
                local pos = v:get_thrusterpos(th)
                if pos.y < 998.0 then 
                    thrust_val = thrust_val + (v:get_thrustermax0(th) * main_throttle)
                end
            end
        end
    end
    local thrust_mn = thrust_val / 1e6
    local twr_val = thrust_val / (mass * 9.80665)

    -- 3. FORMAT AS CSV (Using %d for the integer prog_code)
    local csv_line = string.format("%.1f,%d,%.2f,%.0f,%.2f,%.2f,%.2f", 
                                   met, prog_code, alt, mass, pitch, thrust_mn, twr_val)
    
    -- 4. WRITE TO FILE
    log_to_file(csv_line)
end

-- =========================================================
-- INPUT HANDLING
-- =========================================================
function consumebutton(bt, event)
    if event % PANEL_MOUSE.LBPRESSED == PANEL_MOUSE.LBDOWN then
        if bt == 0 then
            page = page - 1
            if page < 1 then page = total_pages end
            mfd:invalidate_display()
            return true
        elseif bt == 1 then
            page = page + 1
            if page > total_pages then page = 1 end
            mfd:invalidate_display()
            return true
        elseif bt == 2 then
            is_logging = not is_logging -- Toggle state
            return true
        end
    end
    return false
end

function consumekeybuffered(key)
    if key == OAPI_KEY.P then 
        page = page - 1; if page < 1 then page = total_pages end; 
        mfd:invalidate_display(); return true
    elseif key == OAPI_KEY.N then 
        page = page + 1; if page > total_pages then page = 1 end; 
        mfd:invalidate_display(); return true
    elseif key == OAPI_KEY.L then 
        is_logging = not is_logging -- Toggle state
        return true
    end
    return false
end

-- =========================================================
-- PAGE RENDERING ROUTINES (HARDENED)
-- =========================================================
local function draw_text(skp, line, text)
    skp:text(cw * 2, ch * (2 + line * 1.2), text, string.len(text))
end

local function update_page_1(skp, v)
    local ap_on, prog_code, vinka_status = get_vinka_info(v)

    draw_text(skp, 0, "--- GUIDANCE STATUS ---")
    if ap_on == 1 then skp:set_textcolor(0xFFFF00) else skp:set_textcolor(0x808080) end
    draw_text(skp, 1, string.format("%s", vinka_status))
    skp:set_textcolor(0x00FF00)

    draw_text(skp, 3, "--- FLIGHT DYNAMICS ---")
    draw_text(skp, 4, string.format("MET:      %.1f s", oapi.get_simtime()))
    draw_text(skp, 5, string.format("Altitude: %.2f m", v:get_altitude()))
    draw_text(skp, 6, string.format("Airspeed: %.2f m/s", v:get_airspeed()))
    draw_text(skp, 7, string.format("Pitch:    %.2f deg", v:get_pitch() * 57.2957))
    
    -- DYNAMIC FUEL CALCULATION (Immune to tank deletion)
    local total_mass = v:get_mass()
    local empty_mass = v:get_emptymass()
    local fuel_mass = total_mass - empty_mass
    draw_text(skp, 9, string.format("Total Mass: %.0f kg", total_mass))
    draw_text(skp, 10, string.format("Fuel Mass:  %.0f kg", fuel_mass))

    -- DYNAMIC ENGINE POLLING (Only queries THGROUP_MAIN)
    local eng_count = v:get_groupthrustercount(0) 
    local total_thrust = 0
    local max_thrust = 0
    local main_throttle = v:get_thrustergrouplevel(0) or 0

    for i = 0, eng_count - 1 do
        local th = v:get_groupthruster(0, i)
        if th then
            local th_max = v:get_thrustermax0(th)
            total_thrust = total_thrust + (main_throttle * th_max)
            max_thrust = max_thrust + th_max
        end
    end

    local twr_val = 0
    if total_mass > 0 then twr_val = total_thrust / (total_mass * 9.80665) end

    draw_text(skp, 12, string.format("Main Engs:  %d Active", eng_count))
    draw_text(skp, 13, string.format("Thrust:     %.2f MN", total_thrust / 1e6))
    draw_text(skp, 14, string.format("TWR:        %.2f", twr_val))
end

local function update_page_2(skp, v)
    draw_text(skp, 0, "--- VISUAL MESHES (off=) ---")
    local count = v:get_meshcount()
    local max_lines = math.floor(disph / (ch * 1.2)) - 4
    local line = 0

    for i = 0, count - 1 do
        if line > max_lines then break end
        -- DYNAMIC SAFETY: Re-check count inside loop!
        if i < v:get_meshcount() then
            local pos = v:get_meshoffset(i)
            if pos then
                draw_text(skp, 2 + line, string.format("M[%02d]: X=%6.2f Y=%6.2f Z=%6.2f", i, pos.x, pos.y, pos.z))
                line = line + 1
            end
        end
    end
end

local function update_page_3(skp, v)
    draw_text(skp, 0, "--- ENGINE POS (eng_off=) ---")
    local t_count = v:get_thrustercount()
    local max_lines = math.floor(disph / (ch * 1.2)) - 4
    local line = 0

    for i = 0, t_count - 1 do
        if line > max_lines then break end
        if i < v:get_thrustercount() then
            local th = v:get_thrusterhandle(i)
            if th then
                local pos = v:get_thrusterpos(th)
                -- FILTER OUT PHANTOM DATA BUS (> 900 Y)
                if pos and pos.y < 900.0 then
                    draw_text(skp, 2 + line, string.format("TH[%02d]: X=%5.2f Y=%5.2f Z=%6.2f", i, pos.x, pos.y, pos.z))
                    line = line + 1
                end
            end
        end
    end
end

local function update_page_4(skp, v)
    draw_text(skp, 0, "--- THRUST SETTINGS ---")
    local t_count = v:get_thrustercount()
    local max_lines = math.floor(disph / (ch * 1.2)) - 4
    local line = 0

    for i = 0, t_count - 1 do
        if line > max_lines then break end
        if i < v:get_thrustercount() then
            local th = v:get_thrusterhandle(i)
            if th then
                local pos = v:get_thrusterpos(th)
                -- FILTER OUT PHANTOM DATA BUS
                if pos and pos.y < 900.0 then
                    local max_th = v:get_thrustermax0(th)
                    local lvl = v:get_thrusterlevel(th)
                    local id = "RCS"
                    if max_th > 15000000 then id = "SRB"
                    elseif max_th > 1800000 then id = "RS25" 
                    elseif max_th > 100000 then id = "ICPS" end

                    draw_text(skp, 2 + line, string.format("TH[%02d] %-4s: %.0f N [%3.0f%%]", i, id, max_th, lvl * 100))
                    line = line + 1
                end
            end
        end
    end
end

local function update_page_5(skp, v)
    draw_text(skp, 0, "--- MISC SETTINGS ---")
    local pmi = v:get_pmi()
    draw_text(skp, 2, string.format("Empty Mass:  %.0f kg", v:get_emptymass()))
    draw_text(skp, 3, string.format("Size/Radius: %.2f m", v:get_size()))
    draw_text(skp, 5, string.format("PMI X: %.2f", pmi.x))
    draw_text(skp, 6, string.format("PMI Y: %.2f", pmi.y))
    draw_text(skp, 7, string.format("PMI Z: %.2f", pmi.z))
end

local function update_page_6(skp, v)
    draw_text(skp, 0, "--- PROPELLANT DIAGNOSTICS ---")
    local p_count = v:get_propellantcount()
    local max_lines = math.floor(disph / (ch * 1.2)) - 4
    local line = 2

    for i = 0, p_count - 1 do
        if line > max_lines then break end
        if i < v:get_propellantcount() then
            local ph = v:get_propellanthandle(i)
            if ph then
                local mass = v:get_propellantmass(ph)
                local max_mass = v:get_propellantmaxmass(ph)
                
                -- Only draw valid tanks that have a max capacity
                if max_mass > 0 then
                    local id = "TANK"
                    if max_mass > 600000 and max_mass < 650000 then id = "SRB" 
                    elseif max_mass > 900000 then id = "CORE" 
                    elseif max_mass > 20000 and max_mass < 30000 then id = "ICPS" end

                    draw_text(skp, line, string.format("P[%02d] %-4s: %10.3f / %.0f kg", i, id, mass, max_mass))
                    line = line + 1
                end
            end
        end
    end
end

-- =========================================================
-- MAIN MFD UPDATE LOOP
-- =========================================================
function update(skp)
    -- MOVE THIS TO THE TOP: Grab the vessel immediately so it exists for the logger!
    local v = vessel.get_focusinterface()
    if v == nil then return false end

    skp:set_font(mfd:get_defaultfont(0))
    skp:set_textcolor(0x00FF00) -- Green text
    ch, cw = skp:get_charsize()

    mfd:set_title(skp, string.format("SLS SPY [%d/%d]", page, total_pages))

    -- ==========================================
    -- CONTINUOUS LOGGING LOGIC & UI
    -- ==========================================
    if is_logging then
        -- 1. Draw Red * LOGGING * in the top right corner
        skp:set_textcolor(0x0000FF) -- Red (Orbiter uses BBGGRR format)
        local log_msg = "* LOGGING *"
        skp:text(dispw - (cw * string.len(log_msg)) - cw, ch, log_msg, string.len(log_msg))
        skp:set_textcolor(0x00FF00) -- Reset back to Green for the rest of the MFD
        
        -- 2. Timer Check: Only log if 1 second has passed
        local current_time = oapi.get_simtime()
        if current_time - last_log_time >= log_interval then
            log_telemetry_csv(v) -- Changed to the new CSV logger!
            last_log_time = current_time
        end
    end

    -- Route to correct page renderer
    if page == 1 then update_page_1(skp, v)
    elseif page == 2 then update_page_2(skp, v)
    elseif page == 3 then update_page_3(skp, v)
    elseif page == 4 then update_page_4(skp, v)
    elseif page == 5 then update_page_5(skp, v)
    elseif page == 6 then update_page_6(skp, v)
    end

    -- Force MFD to refresh next frame so data updates live
    mfd:invalidate_display()
    
    return true
end

