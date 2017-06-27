###############################################################################
# SETUP
###############################################################################

# global settings:
axes location Off
color Display Background white
display projection Orthographic
display depthcue off
display ambientocclusion on
display rendermode GLSL
color change rgb 7 1.000000 1.000000 1.000000

# load molecule:
mol delete top
if { [molinfo top] == -1 } {
    mol new pr.gro
}


###############################################################################
# BASIC PROTEIN
###############################################################################

# delete default representation:
mol delrep 0 top

# protein:
mol addrep top
mol modselect 0 top (protein)
mol modstyle 0 top NewRibbons 0.3 10.0 4.1 0
mol modcolor 0 top ColorID 6
mol modmaterial 0 top AOEdgy


###############################################################################
# PORE LINING RESIDUES
###############################################################################

# process for importing data:
proc import_pore_lining {filename} {

    # prepare lists as data containers:
    set res_id {}
    set pore_lining {}
    set pore_facing {}

    # parse csv file line by line:
    set linenumber 1;
    set csvfile [open $filename r]
    while { [gets $csvfile line] >= 0 } {

        # assume first line is header:
        if { $linenumber == 1 } {

            puts "$line"

        } else {

            # split line at whitespaces:
            set linespl [regexp -all -inline {\S+} $line]

            # add data to containers:
            lappend res_id [lindex $linespl 1]
            lappend pore_lining [lindex $linespl 5]
            lappend pore_facing [lindex $linespl 6]
        }

        # increment line counter:
        incr linenumber
    }
    close $csvfile

    # return data:
    return [list $res_id $pore_lining $pore_facing]
}


# read pore lining data from file:
set filename "res_mapping.dat"
set pore_lining_data [import_pore_lining $filename]


# seperate into several containers:
set res_id [lindex $pore_lining_data 0]
set pore_lining [lindex $pore_lining_data 1]
set pore_facing [lindex $pore_lining_data 2]


# loop over all residues:
foreach rid $res_id pl $pore_lining pf $pore_facing {
#break

    # is residue pore lining?
    if { $pl == 1 } {
       
        puts "pf = $pf"

        # is residue also pore facing:
        if { $pf == 1 } {

            set repnum [molinfo top get numreps]

            mol addrep top
#            mol modselect $repnum top (resid $rid and name CA)
            mol modselect $repnum top (resid $rid)
#            mol modstyle $repnum top VDW 0.5 12
            mol modstyle $repnum top Licorice
            mol modcolor $repnum top ColorID 32
            mol modmaterial $repnum top AOChalky
        
        } else {

            set repnum [molinfo top get numreps]

            mol addrep top
#            mol modselect $repnum top (resid $rid and name CA)
            mol modselect $repnum top (resid $rid)
#            mol modstyle $repnum top VDW 0.5 12
            mol modstyle $repnum top Licorice
            mol modcolor $repnum top ColorID 27
            mol modmaterial $repnum top AOChalky

        }
    }
}


###############################################################################
# PORE SURFACE IMPORT
###############################################################################

draw color blue
#draw material Transparent
source ~/repos/tcl-obj/src/wobj.tcl


###############################################################################
# PORE HOLE RESULT
###############################################################################


# import OBJ data:
set filename pore.obj
set obj [WOBJ::import_wavefront_obj $filename]

# draw OBJ mesh:
WOBJ::draw_wavefront_obj $obj






