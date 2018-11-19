# -*- mode: conf-unix; -*-

#################################
#@description PMT HAMAMATSU R6594
#################################


##########################################################################
[name="PMT_HAMAMATSU_R6594.dynodes" type="geomtools::simple_shaped_model"]
#@config Simplified PMT dynodes

#######################
# Geometry parameters #
#######################

shape_type  : string = "tube"
outer_r     : real   = 15.0
inner_r     : real   = 14.5
z           : real   = 80
length_unit : string = "mm"

#######################
# Material parameters #
#######################

material.ref      : string = "std::copper"

#########################
# Visibility parameters #
#########################

visibility.hidden : boolean = 0
visibility.color  : string  = "orange"


#######################################################################
[name="PMT_HAMAMATSU_R6594.base" type="geomtools::simple_shaped_model"]
#@config Simplified PMT base

#######################
# Geometry parameters #
#######################

shape_type  : string = "cylinder"
length_unit : string = "mm"
r            : real   = 25.5
z            : real   = 30.0

#######################
# Material parameters #
#######################

material.ref      : string = "std::delrin"

#########################
# Visibility parameters #
#########################

visibility.hidden : boolean = 0
visibility.color  : string  = "orange"


##################################################################
[name="PMT_HAMAMATSU_R6594" type="geomtools::simple_shaped_model"]

#@config The configuration parameters for the PMT's bulb and its contents
# Simplified PMT glass envelop and vacuum interior

#######################
# Geometry parameters #
#######################

#@description The default implicit length unit
length_unit : string = "mm"

#@description The shape of the PMT's bulb
shape_type  : string = "polycone"

#@description The polycone build mode
build_mode  : string = "datafile"

#@description The polycone coordinates filename
datafile    : string = "@falaise:common/geometry/pmt/1.0/hamamatsu_R6594/hamamatsu_R6594_shape.data"

#@description The 'filled' mode to build the model
filled_mode : string = "by_envelope"

#@description The label of the PMT's bulb volume as daughter volume of the model's envelope
filled_label : string = "bulb"

#######################
# Material parameters #
#######################

#@description The material name
material.ref        : string = "glass"

#@description The inner material name
material.filled.ref : string = "vacuum"

#########################
# Visibility parameters #
#########################

#@description The visibility hidden flag for the display
visibility.hidden           : boolean = 1

#@description The visibility hidden flag for the envelope
visibility.hidden_envelop   : boolean = 1

#@description The recommended color for the display
visibility.color            : string  = "cyan"

#@description The visibility hidden flag for the daughters volumes
visibility.daughters.hidden : boolean = 1

###########################
# Internal/daughter items #
###########################

#@description The list of daughter volumes by labels
internal_item.filled.labels            : string[1] = "dynodes"

#@description The placement of the "dynodes" daughter volume
internal_item.filled.placement.dynodes : string  = "0 0 -30 (mm)"

#@description The model of the "dynodes" daughter volume
internal_item.filled.model.dynodes     : string  = "PMT_HAMAMATSU_R6594.dynodes"

# end
