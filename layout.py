#!/Applications/Kicad/kicad.app/Contents/Frameworks/Python.framework/Versions/Current/bin/python
# -*- coding: utf-8 -*-
import os
import sys
from math import *
import cmath
import argparse

parser = argparse.ArgumentParser()
parser.add_argument('-p', "--path", action='store', required=True, help="Path to kicad_pcb file")
parser.add_argument('--delete-all-traces', action='store_true', help='Delete All Traces in the pcbnew and then exit')
parser.add_argument('--delete-all-drawings', action='store_true', help='Delete all lines & texts in the pcbnew and then exit')
parser.add_argument('--delete-short-traces', action='store_true', help='Delete traces of zero or very small length and exit')
parser.add_argument('--dry-run', action='store_true', help='Don\'t save results')
parser.add_argument('-v', dest='verbose', action='count', help="Verbose")
parser.add_argument('--skip-traces', action='store_true', help='Don\'t add traces')
args = parser.parse_args()
pcb_path = os.path.abspath(args.path)

sys.path.insert(0, "/Applications/Kicad/kicad.app/Contents/Frameworks/python/site-packages/")
sys.path.append("/Library/Python/2.7/site-packages")
sys.path.append("/Library/Frameworks/Python.framework/Versions/2.7/lib/python2.7/site-packages")
os.chdir(os.path.dirname(os.path.realpath(__file__)))

import kicad
import pcbnew

from kicad.pcbnew import Board
board = Board.from_file(pcb_path)

layertable = {}

numlayers = pcbnew.PCB_LAYER_ID_COUNT
for i in range(numlayers):
	layertable[board._obj.GetLayerName(i)] = i

netcodes = board._obj.GetNetsByNetcode()
netnames = {}
for netcode, net in netcodes.items():
	netnames[net.GetNetname()] = net

class Point(object):
	@classmethod
	def fromWxPoint(cls, wxPoint):
		return Point(wxPoint.x / pcbnew.IU_PER_MM, wxPoint.y / pcbnew.IU_PER_MM)

	@classmethod
	def fromComplex(cls, c):
		return Point(c.real, c.imag)

	def __init__(self, x, y):
		self.x = x
		self.y = y

	def wxPoint(self):
		return pcbnew.wxPoint(self.x * pcbnew.IU_PER_MM , self.y * pcbnew.IU_PER_MM)

	def translate(self, vec):
		self.x += vec.x
		self.y += vec.y

	def translated(self, vec):
		return Point(self.x+vec.x, self.y+vec.y)

	def __getitem__(self, i):
		if i == 0:
			return self.x
		if i == 1:
			return self.y
		raise IndexError("index out of range")

	def distance_to(self, other):
		return sqrt((self.x - other.x) ** 2 + (self.y - other.y) ** 2)

	def __str__(self):
		return "(%0.2f, %0.2f)" % (self.x, self.y)


def draw_segment(start, end, layer='F.SilkS', width=0.15):
    line = pcbnew.DRAWSEGMENT(board._obj)
    board._obj.Add(line)
    line.SetShape(pcbnew.S_SEGMENT)
    line.SetStart(start.wxPoint())
    line.SetEnd(end.wxPoint())
    print("LINE from (%f, %f) to (%f, %f)" % (start.x, start.y, end.x, end.y))
    line.SetLayer(layertable[layer])
    line.SetWidth(int(width * pcbnew.IU_PER_MM))
    return line


def draw_circle(center, radius, layer='F.SilkS', width=0.15):
    circle = pcbnew.DRAWSEGMENT(board._obj)
    board._obj.Add(circle)
    circle.SetShape(pcbnew.S_CIRCLE)
    circle.SetCenter(center.wxPoint())
    center.translate(Point(0, radius))
    circle.SetArcStart(center.wxPoint())
    circle.SetLayer(layertable[layer])
    circle.SetWidth(int(width * pcbnew.IU_PER_MM))
    return circle
    

def draw_arc_1(center, radius, start_angle, stop_angle,
                 layer='F.SilkS', width=0.15):
    
    print("Drawing arc with center at (%f,%f), radius: %f, angle %f -> %f" % (center.x, center.y, radius, start_angle, stop_angle))
    arcStartPoint = radius * cmath.exp(start_angle * 1j)
    arcStartPoint = center.translated(Point.fromComplex(arcStartPoint))
    print("arcStartPoint %s" % str(arcStartPoint));
    angle = stop_angle - start_angle
    arc = pcbnew.DRAWSEGMENT(board._obj)
    board._obj.Add(arc)
    arc.SetShape(pcbnew.S_ARC)
    arc.SetCenter(center.wxPoint())
    arc.SetArcStart(arcStartPoint.wxPoint())
    arc.SetAngle(angle * 180/pi * 10)
    arc.SetLayer(layertable[layer])
    arc.SetWidth(int(width * pcbnew.IU_PER_MM))
    return arc

def draw_arc_2(center, arcStartPoint, totalAngle,
                 layer='F.SilkS', width=0.15):
    print("Drawing arc with center: (%f, %f), arc point at (%f,%f), total angle %f" % (center.x, center.y, arcStartPoint.x, arcStartPoint.y, totalAngle))
    arc = pcbnew.DRAWSEGMENT(board._obj)
    board._obj.Add(arc)
    arc.SetShape(pcbnew.S_ARC)
    arc.SetCenter(center.wxPoint())
    arc.SetArcStart(arcStartPoint.wxPoint())
    arc.SetAngle(totalAngle * 180/pi * 10)
    arc.SetLayer(layertable[layer])
    arc.SetWidth(int(width * pcbnew.IU_PER_MM))
    return arc

def add_copper_trace(start, end, net):
	track = pcbnew.TRACK(board._obj)
	track.SetStart(start)
	track.SetEnd(end)
	track.SetLayer(layertable["F.Cu"])
	track.SetWidth(int(.25 * 10**6))
	track.SetNet(net)
	board._obj.Add(track)
	print("adding track from {} to {} on net {}".format(start, end, net.GetNetname()))
	return track

prev_module = None
def clear_tracks_for_module(module): 
	for pad in module._obj.Pads():
		pad_position = pad.GetPosition()
		tracks = board._obj.GetTracks()
		for t in tracks:
			track_start = t.GetStart()
			track_end = t.GetEnd()
			thresh = 100 # millionths of an inch?
			d1 = sqrt((track_start.x - pad_position.x)**2 + (track_start.y - pad_position.y)**2)
			d2 = sqrt((track_end.x - pad_position.x)**2 + (track_end.y - pad_position.y)**2)

			if d1 < thresh or d2 < thresh:
			   print("Deleting old track for module %s pad %s" % (module.reference, pad.GetPadName()))
			   board._obj.Delete(t)

def place(module, point, orientation):
	global prev_module
	from kicad.util.point import Point2D

	print("Placing module %s at point %f,%f orientation %f" % (module.reference, point.x, point.y, orientation));
	module.position = Point2D(point.x, point.y)
	orientation *= 180/pi
	orientation *= 10 # kicad *shrug*
	# print(dir(module)) 
	if orientation is not None:
		module._obj.SetOrientation(orientation)

	# Add tracks from the previous  module, connecting pad 5 to pad 2 and pad 4 to pad 3
	if not args.skip_traces and prev_module is not None:
		pad_map = {"2": "5", "3": "4"} # connect SCK and SD*
		for prev_pad in prev_module._obj.Pads():
			if prev_pad.GetPadName() in pad_map:
				tracks = board._obj.TracksInNet(prev_pad.GetNet().GetNet())
				if tracks:
					# skip pad, already has traces
					if args.verbose:
						print("Skipping pad, already has tracks: {}".format(prev_pad))
					continue
				# for net in board._obj.TracksInNet(prev_pad.GetNet().GetNet()):
				# 	board._obj.Delete(t)

				# then connect the two pads
				for pad in module._obj.Pads():
					if pad.GetPadName() == pad_map[prev_pad.GetPadName()]:
						start = pcbnew.wxPoint(prev_pad.GetPosition().x, prev_pad.GetPosition().y)
						end = pcbnew.wxPoint(pad.GetPosition().x, pad.GetPosition().y)
						print("Adding track from module {} pad {} to module {} pad {}".format(prev_module.reference, prev_pad.GetPadName(), module.reference, pad.GetPadName()))
						add_copper_trace(start, end, pad.GetNet())

				 # FIXME: Draw GND and +5V traces for circle

	prev_module = module

from collections import namedtuple
Spoke = namedtuple('Spoke', ['angle', 'arrow', 'cross'])

def layout_trans_symbol():
	centerx = 90
	centery = 90

	circle_radius = 20#mm
	circle_npixels = 34

	cross_npixels = 4 # should be even
	arrow_npixels = 6 # should be even

	spokes = [Spoke(pi/2, False, True),
	          Spoke(5*pi/4, True, True),
	          Spoke(7*pi/4, True, False)]
	spoke_length = 8

	pixel_spacing = 3.5#mm

	def circle_pt(theta, radius):
		return Point(centerx+radius*cos(theta), centery+radius*sin(theta))

	# delete old edge cuts
	for drawing in board._obj.DrawingsList():
		if drawing.IsOnLayer(layertable['Edge.Cuts']):
			board._obj.Delete(drawing)
			print("Deleting edge cut drawing")

	# draw edge.cuts
	def draw_edge_cuts():
		edge_cut_radius = circle_radius + 4
		edge_cut_line_thickness = 0.05
		spoke_arclen = pi/3
		spoke_length = 20
		# spacer arcs are not uniform. the two spokes should point up at 45º and 135º, and the lower spoke points straight down at 270º (counter-clockwise coordinates)
		# btw kicad arcs go clockwise
		
		spacer_arcs = [abs(spokes[(i+1)%len(spokes)].angle - spokes[i].angle - spoke_arclen + (2 * pi if i+1>=len(spokes) else 0)) for i in range(len(spokes))]
		arc_accum = spokes[0].angle + spoke_arclen/2

		arc_start = None
		for i in range(len(spacer_arcs)):
			circle_arc = draw_arc_1(Point(centerx, centery), edge_cut_radius, arc_accum, arc_accum + spacer_arcs[i], layer='Edge.Cuts', width=edge_cut_line_thickness)
			if arc_start is not None:
				circle_arc.SetArcStart(arc_start.wxPoint())

			arc_accum+=spacer_arcs[i]

			spoke_base = circle_pt(arc_accum+spoke_arclen/2, edge_cut_radius)
			spoke_tip = circle_pt(arc_accum+spoke_arclen/2, edge_cut_radius + spoke_length)
			spoke_offset = Point(spoke_tip.x - spoke_base.x, spoke_tip.y - spoke_base.y)
			
			spoke_base_1 = Point.fromWxPoint(circle_arc.GetArcEnd())
			assert(spoke_base_1.distance_to(circle_pt(arc_accum, edge_cut_radius)) < 0.01)

			spoke_edge_1 = Point(spoke_base_1.x + spoke_offset.x, spoke_base_1.y + spoke_offset.y)

			spoke_base_2 = circle_pt(arc_accum+spoke_arclen, edge_cut_radius)
			spoke_edge_2 = Point(spoke_base_2.x + spoke_offset.x, spoke_base_2.y + spoke_offset.y)

			spoke_center = Point((spoke_edge_1.x + spoke_edge_2.x) / 2, (spoke_edge_1.y + spoke_edge_2.y) / 2)
			spoke_radius = sqrt((spoke_edge_1.x - spoke_edge_2.x)**2 + (spoke_edge_1.y - spoke_edge_2.y)**2) / 2 
			spoke_arc = draw_arc_2(spoke_center, spoke_edge_1, pi, layer='Edge.Cuts', width=edge_cut_line_thickness)

			spoke_arc.SetArcStart(spoke_edge_1.wxPoint())
			spoke_arc_end = Point.fromWxPoint(spoke_arc.GetArcEnd())
			assert(spoke_edge_2.distance_to(spoke_arc_end) < 0.01)
			spoke_edge_2 = spoke_arc_end

			spoke_side_1 = draw_segment(spoke_base_1, spoke_edge_1, layer='Edge.Cuts', width=edge_cut_line_thickness)
			spoke_side_2 = draw_segment(spoke_base_2, spoke_edge_2, layer='Edge.Cuts', width=edge_cut_line_thickness)

			arc_start = spoke_base_2

			arc_accum += spoke_arclen

	draw_edge_cuts()

	# remove previous tracks
	modules = dict(zip((m.reference for m in board.modules), (m for m in board.modules)))

	for module in modules.values():
		if module.reference.startswith('D'):
			clear_tracks_for_module(module)

	# place pixels
	def place_pixels():
		pix = 1
		module = modules["D%i"% pix]

		def place_spoke(pix, module, theta, arrow, cross):
			print("DO SPOKE at theta %f, circle_radius = %f" % (theta, circle_radius))
			
			pixel_orientation = pi - theta
			for i in range(spoke_length):
				pt = circle_pt(theta, circle_radius + pixel_spacing*(i+1))
				place(module, Point(pt.x, pt.y), pixel_orientation)
				pix+=1
				module = modules["D%i"% pix]

				# need to draw the cross & arrow after finishing the line of the spoke, since there's no room for traces to come out and back into that line of leds
				if i == spoke_length - 1:
					# TODO factor arrow and cross together since they are so similar
					# lol this todo is the trans agenda
					if arrow:
						print("do arrow")
						pt = circle_pt(theta, circle_radius + pixel_spacing*spoke_length)

						for arrow_px in range(arrow_npixels):
							direction = 1 if arrow_px < arrow_npixels>>1 else -1
							tilt = pi/2 + direction * pi/4
							side_idx = 1+arrow_px%(arrow_npixels>>1)
							offset = direction * (.75 + side_idx * pixel_spacing) * cmath.exp((theta + tilt) * 1j) 
							adjust = .75 * cmath.exp((theta + direction * pi/4) * 1j)
							place(module, pt.translated(Point.fromComplex(offset + adjust)), pixel_orientation + tilt)
							pix+=1
							module = modules["D%i"% pix]

					if cross:
						print("do cross")
						cross_location = (3/8. if arrow else 5/8.) * float(spoke_length)
						pt = circle_pt(theta, circle_radius + pixel_spacing*cross_location)
						
						for cross_px in range(cross_npixels):
							direction = -1 if cross_px < cross_npixels>>1 else 1
							side_idx = 1+cross_px%(cross_npixels>>1)
							offset = direction * side_idx * pixel_spacing * cmath.exp((theta + pi/2) * 1j)
							place(module, pt.translated(Point.fromComplex(offset)), pixel_orientation)
							pix+=1
							module = modules["D%i"% pix]
			return pix, module


		spokes_to_draw = [s for s in spokes]

		last_theta = None
		circle_index = 1
		while module is not None:
			theta = pi/2 + 2 * pi * circle_index / circle_npixels
			if theta > 2*pi:
				theta -= 2*pi

			for spoke in spokes_to_draw:
				if last_theta is not None and last_theta <= spoke.angle and theta >= spoke.angle:
					# start spoke!
					print("starting spoke at pix %i" % pix)
					pix, module = place_spoke(pix, module, spoke.angle, spoke.arrow, spoke.cross)
					print("finished spoke, pix now %i" % pix)
					spokes_to_draw.remove(spoke)

			if circle_index > circle_npixels:
				break

			x = centerx + cos(theta) * circle_radius
			y = centery + sin(theta) * circle_radius
			orientation = (pi/2 - theta)
			print("Theta now %f, last_theta %f, spoke_angles = %s" % (theta, (last_theta if last_theta is not None else 0.0), str(spokes)))
			place(module, Point(x, y), orientation)

			circle_index += 1
			pix += 1
			module = modules["D%i"% pix]

			last_theta = theta

	place_pixels()



def save():
	print("Saving...")
	backup_path = pcb_path + ".layoutbak"
	if os.path.exists(backup_path):
		os.unlink(backup_path)
	os.rename(pcb_path, backup_path)
	print("  Backing up '%s' to '%s'..." % (pcb_path, backup_path))
	assert board._obj.Save(pcb_path)
	print("Saved!")

if args.delete_all_traces:
	tracks = board._obj.GetTracks()
	for t in tracks:
		print("Deleting track {}".format(t))
		board._obj.Delete(t)
if args.delete_all_drawings:
	while list(board._obj.GetDrawings()):
		drawing = list(board._obj.GetDrawings())[0]
		print("Deleting drawing {}".format(drawing))
		board._obj.Delete(drawing)
elif args.delete_short_traces:
	print("FIXME: disabled because it removes vias")
	exit(1)
	tracks = board._obj.GetTracks()
	for t in tracks:
		start = t.GetStart()
		end = t.GetEnd()
		length = sqrt((end.x - start.x)**2 + (end.y - start.y)**2)
		if length < 100: # millionths of an inch
			print("Deleting trace of short length {}in/1000000".format(length))
			board._obj.Delete(t)
else:
	layout_trans_symbol()

if not args.dry_run:
	save()
