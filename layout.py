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

from collections import namedtuple
Point = namedtuple('Point', ['x', 'y'])


def wxPointFrom(tup):
	return pcbnew.wxPoint(tup[0] * pcbnew.IU_PER_MM , tup[1] * pcbnew.IU_PER_MM)

def draw_segment(start, end, layer='F.SilkS', width=0.15, board=None):
        line = pcbnew.DRAWSEGMENT(board._obj)
        board._obj.Add(line)
        line.SetShape(pcbnew.S_SEGMENT)
        line.SetStart(wxPointFrom(start))
        line.SetEnd(wxPointFrom(end))
        print("LINE from (%f, %f) to (%f, %f)" % (start.x, start.y, end.x, end.y))
        line.SetLayer(layertable[layer])
        line.SetWidth(int(width * pcbnew.IU_PER_MM))


def draw_circle(center, radius, layer='F.SilkS', width=0.15,
                 board=None):
        circle = pcbnew.DRAWSEGMENT(board._obj)
        board._obj.Add(circle)
        circle.SetShape(pcbnew.S_CIRCLE)
        circle.SetCenter(wxPointFrom(center))
        start_coord = wxPointFrom(
            (center[0], center[1] + radius))
        circle.SetArcStart(start_coord)
        circle.SetLayer(layertable[layer])
        circle.SetWidth(int(width * pcbnew.IU_PER_MM))
        

def draw_arc_1(center, radius, start_angle, stop_angle,
                 layer='F.SilkS', width=0.15, board=None):
    
    print("Drawing arc with center at (%f,%f), radius: %f, angle %f -> %f" % (center.x, center.y, radius, start_angle, stop_angle))
    arcStartPoint = radius * cmath.exp(start_angle * 1j)
    print("arcStartPoint %f, %f" % (arcStartPoint.real, arcStartPoint.imag));
    arcStartPoint = wxPointFrom((center.x + arcStartPoint.real, center.y + arcStartPoint.imag))
    angle = stop_angle - start_angle
    arc = pcbnew.DRAWSEGMENT(board._obj)
    board._obj.Add(arc)
    arc.SetShape(pcbnew.S_ARC)
    arc.SetCenter(wxPointFrom(center))
    arc.SetArcStart(arcStartPoint)
    arc.SetAngle(angle * 180/pi * 10)
    arc.SetLayer(layertable[layer])
    arc.SetWidth(int(width * pcbnew.IU_PER_MM))

def draw_arc_2(center, arcStartPoint, totalAngle,
                 layer='F.SilkS', width=0.15, board=None):
    print("Drawing arc with center: (%f, %f), arc point at (%f,%f), total angle %f" % (center.x, center.y, arcStartPoint.x, arcStartPoint.y, totalAngle))
    arc = pcbnew.DRAWSEGMENT(board._obj)
    board._obj.Add(arc)
    arc.SetShape(pcbnew.S_ARC)
    arc.SetCenter(wxPointFrom(center))
    arc.SetArcStart(wxPointFrom((arcStartPoint.x, arcStartPoint.y)))
    arc.SetAngle(totalAngle * 180/pi * 10)
    arc.SetLayer(layertable[layer])
    arc.SetWidth(int(width * pcbnew.IU_PER_MM))

def add_copper_trace(start, end, net):
	track = pcbnew.TRACK(board._obj)
	track.SetStart(start)
	track.SetEnd(end)
	track.SetLayer(layertable["F.Cu"])
	track.SetWidth(int(.25 * 10**6))
	track.SetNet(net)
	board._obj.Add(track)
	print("adding track from {} to {} on net {}".format(start, end, net.GetNetname()))

prev_module = None
def place(module, point, orientation = None):
	global prev_module

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

	print("Placing module %s at point %f,%f angle %f" % (module.reference, point.x, point.y, orientation));
	module.position = point
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

	prev_module = module


num_leds = 96	

def layout_trans_symbol():
	from kicad.util.point import Point2D

	centerx = 90
	centery = 90

	circle_radius = 20#mm
	circle_npixels = 32

	def circle_pt(theta, radius):
		return Point(centerx+radius*cos(theta), centery+radius*sin(theta))

	# delete old edge cuts
	for drawing in board._obj.DrawingsList():
		if drawing.IsOnLayer(layertable['Edge.Cuts']):
			board._obj.Delete(drawing)
			print("Deleting edge cut drawing")

	# draw edge.cuts
	edge_cut_radius = circle_radius + 4
	spoke_arc = pi/3
	spoke_length = 25
	# spacer arcs are not uniform. the two spokes should point up at 45º and 135º, and the lower spoke points straight down at 270º (counter-clockwise coordinates)
	# btw kicad arcs go clockwise
	spoke_angles = [pi/2, 5*pi/4, 7*pi/4]
	
	spacer_arcs = [abs(spoke_angles[(i+1)%len(spoke_angles)] - spoke_angles[i] - spoke_arc + (2 * pi if i+1>=len(spoke_angles) else 0)) for i in range(len(spoke_angles))]
	arc_accum = spoke_angles[0] + spoke_arc/2

	for i in range(len(spacer_arcs)):
		draw_arc_1(Point(centerx, centery), edge_cut_radius, arc_accum, arc_accum + spacer_arcs[i], layer='Edge.Cuts', width=1, board=board)
		arc_accum+=spacer_arcs[i]

		
		spoke_base = circle_pt(arc_accum+spoke_arc/2, edge_cut_radius)
		spoke_tip = circle_pt(arc_accum+spoke_arc/2, edge_cut_radius + spoke_length)
		spoke_offset = Point(spoke_tip.x - spoke_base.x, spoke_tip.y - spoke_base.y)
		
		spoke_base_1 = circle_pt(arc_accum, edge_cut_radius)
		spoke_edge_1 = Point(spoke_base_1.x + spoke_offset.x, spoke_base_1.y + spoke_offset.y)

		spoke_base_2 = circle_pt(arc_accum+spoke_arc, edge_cut_radius)
		spoke_edge_2 = Point(spoke_base_2.x + spoke_offset.x, spoke_base_2.y + spoke_offset.y)

		draw_segment(spoke_base_1, spoke_edge_1, layer='Edge.Cuts', width=1, board=board)
		draw_segment(spoke_base_2, spoke_edge_2, layer='Edge.Cuts', width=1, board=board)

		spoke_center = Point((spoke_edge_1.x + spoke_edge_2.x) / 2, (spoke_edge_1.y + spoke_edge_2.y) / 2)
		spoke_radius = sqrt((spoke_edge_1.x - spoke_edge_2.x)**2 + (spoke_edge_1.y - spoke_edge_2.y)**2) / 2 
		draw_arc_2(spoke_center, spoke_edge_1, pi, layer='Edge.Cuts', width=1, board=board)

		arc_accum += spoke_arc

	# place pixels
	modules = dict(zip((m.reference for m in board.modules), (m for m in board.modules)))

	for pix in range(1, circle_npixels+1):
		theta = 2 * pi * pix / circle_npixels

		module = modules["D%i" % pix]
		if not module: continue

		x = centerx + cos(theta) * circle_radius
		y = centery + sin(theta) * circle_radius
		orientation = (-180*theta/pi + 90) * 10

		place(module, Point2D(x, y), orientation)


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
