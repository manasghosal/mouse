# -*- coding: utf-8 -*-
"""
Created on Tue May 14 00:14:28 2019

@author: Admin
"""
from ctypes import *

import dlib
import cv2
import argparse, os, random
import torch
import torch.nn as nn
import torch.nn.functional as F
import torchvision
from torchvision import datasets, transforms
import pandas as pd
import numpy as np
from model import model_static
from PIL import Image
from PIL import ImageDraw
from PIL import ImageFont
from colour import Color
import time
import win32pipe, win32file, pywintypes

parser = argparse.ArgumentParser()

parser.add_argument('--video', type=str, help='input video path. live cam is used when not specified')
parser.add_argument('--face', type=str, help='face detection file path. dlib face detector is used when not specified')
parser.add_argument('--model_weight', type=str, help='path to model weights file', default='data/model_weights.pkl')
parser.add_argument('--jitter', type=int, help='jitter bbox n times, and average results', default=0)
parser.add_argument('-save_vis', help='saves output as video', action='store_true')
parser.add_argument('-save_text', help='saves output as text', action='store_true')
parser.add_argument('-display_off', help='do not display frames', action='store_true')

args = parser.parse_args()

CNN_FACE_MODEL = 'data/mmod_human_face_detector.dat' # from http://dlib.net/files/mmod_human_face_detector.dat.bz2


def bbox_jitter(bbox_left, bbox_top, bbox_right, bbox_bottom):
    cx = (bbox_right+bbox_left)/2.0
    cy = (bbox_bottom+bbox_top)/2.0
    scale = random.uniform(0.8, 1.2)
    bbox_right = (bbox_right-cx)*scale + cx
    bbox_left = (bbox_left-cx)*scale + cx
    bbox_top = (bbox_top-cy)*scale + cy
    bbox_bottom = (bbox_bottom-cy)*scale + cy
    return bbox_left, bbox_top, bbox_right, bbox_bottom


def drawrect(drawcontext, xy, outline=None, width=0):
    (x1, y1), (x2, y2) = xy
    points = (x1, y1), (x2, y1), (x2, y2), (x1, y2), (x1, y1)
    drawcontext.line(points, fill=outline, width=width)

def send_ipc(hPipe, score):
    v = round(score, 3)
    strv1 = str(v)
    strv2 = str(v)
    strv = strv1 + strv2
    print("ipc_score:", v)
    cbWritten = c_ulong(0)
    l = len(strv)
    fSuccess = windll.kernel32.WriteFile(c_ulonglong(hPipe), c_wchar_p(strv), c_ulong(l), byref(cbWritten), None)
    # if ( (not fSuccess) or (len(strv) != cbWritten.value)):
    #     print("Could not reply to the client's request from the pipe")
    # else:
    #     print("Number of bytes written:", cbWritten.value)



def run(video_path, face_path, model_weight, jitter, vis, display_off, save_text, hPipe):
    # set up vis settings
    red = Color("red")
    colors = list(red.range_to(Color("green"),10))
    font = ImageFont.truetype("data/arial.ttf", 40)
    face_cascade = cv2.CascadeClassifier('haarcascade_frontalface_default.xml')
    #
    # # set up video source
    # if video_path is None:
    cap = cv2.VideoCapture(0)
    #     video_path = 'live.avi'
    # else:
    #     cap = cv2.VideoCapture(video_path)

    # set up output file
    # if save_text:
    #     outtext_name = os.path.basename(video_path).replace('.avi','_output.txt')
    #     f = open(outtext_name, "w")
    # if vis:
    #     outvis_name = os.path.basename(video_path).replace('.avi','_output.avi')
    #     imwidth = int(cap.get(3)); imheight = int(cap.get(4))
    #     outvid = cv2.VideoWriter(outvis_name,cv2.VideoWriter_fourcc('M','J','P','G'), cap.get(5), (imwidth,imheight))

    # set up face detection mode
    # if face_path is None:
    #     facemode = 'DLIB'
    # else:
    #     facemode = 'GIVEN'
    #     column_names = ['frame', 'left', 'top', 'right', 'bottom']
    #     df = pd.read_csv(face_path, names=column_names, index_col=0)
    #     df['left'] -= (df['right']-df['left'])*0.2
    #     df['right'] += (df['right']-df['left'])*0.2
    #     df['top'] -= (df['bottom']-df['top'])*0.1
    #     df['bottom'] += (df['bottom']-df['top'])*0.1
    #     df['left'] = df['left'].astype('int')
    #     df['top'] = df['top'].astype('int')
    #     df['right'] = df['right'].astype('int')
    #     df['bottom'] = df['bottom'].astype('int')

    # if (cap.isOpened()== False):
    #     print("Error opening video stream or file")
    #     exit()

    # if facemode == 'DLIB':
    #     cnn_face_detector = dlib.cnn_face_detection_model_v1(CNN_FACE_MODEL)
    frame_cnt = 0

    # set up data transformation
    test_transforms = transforms.Compose([transforms.Resize(224), transforms.CenterCrop(224), transforms.ToTensor(),
                                         transforms.Normalize(mean=[0.485, 0.456, 0.406], std=[0.229, 0.224, 0.225])])

    # load model weights
    model = model_static(model_weight)
    model_dict = model.state_dict()
    snapshot = torch.load(model_weight, map_location=torch.device('cpu'))
    model_dict.update(snapshot)
    model.load_state_dict(model_dict)

#    model.cuda()
    model.train(False)
    first_frame = None
    # video reading loop
    while(True):
        score = 0.0
        ret, frame = cap.read()
        #if ret == True:
        if frame is None:
            print("frame is none")
            cap = cv2.VideoCapture(0)
            continue
        gray1 = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)
        gray = cv2.GaussianBlur(gray1, (21, 21), 0)
        if first_frame is None:
            first_frame = gray
            continue
        delta_frame = cv2.absdiff(first_frame, gray)
        thresh_delta = cv2.threshold(delta_frame, 30, 255, cv2.THRESH_BINARY)[1]
        thresh_delta - cv2.dilate(thresh_delta, None, iterations=0)
        major = cv2.__version__.split('.')[0]
        if major == '3':
            _, contours, _ = cv2.findContours(thresh_delta.copy(), cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)
        else:
            contours, _ = cv2.findContours(thresh_delta.copy(), cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)

        for contour in contours:
            if cv2.contourArea(contour) < 1000:
                # print("contour area less than 1000 %d", cv2.contourArea(contour))
                continue

            faces = face_cascade.detectMultiScale(gray1, scaleFactor=1.05, minNeighbors=5)
            for x, y, w, h in faces:
                if w > 50 and h > 50:
                    face = frame[y:y + h, x:x + w]
                    #faceBlob = cv2.dnn.blobFromImage(face, 1.0 / 255, (96, 96), (0, 0, 0), swapRB=True, crop=False)
            # if ret == True:
            # height, width, channels = frame.shape
            # frame = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
            #
            # frame_cnt += 1
            # bbox = []
            # if facemode == 'DLIB':
            #     dets = cnn_face_detector(frame, 1)
            #     for d in dets:
                    l = x
                    r = x+w
                    t = y+h
                    b = y
                    # expand a bit
                    # l -= (r-l)*0.2
                    # r += (r-l)*0.2
                    # t -= (b-t)*0.2
                    # b += (b-t)*0.2
            #         bbox.append([l,t,r,b])
            # elif facemode == 'GIVEN':
            #     if frame_cnt in df.index:
            #         bbox.append([df.loc[frame_cnt,'left'],df.loc[frame_cnt,'top'],df.loc[frame_cnt,'right'],df.loc[frame_cnt,'bottom']])
                    pil_frame = Image.fromarray(face)
            # for b in bbox:
                    #face = faceBlob.crop(([l,r,t,b]))
                    img = test_transforms(pil_frame)
                    img.unsqueeze_(0)
            #     if jitter > 0:
            #         for i in range(jitter):
            #             bj_left, bj_top, bj_right, bj_bottom = bbox_jitter(b[0], b[1], b[2], b[3])
            #             bj = [bj_left, bj_top, bj_right, bj_bottom]
            #             facej = frame.crop((bj))
            #             img_jittered = test_transforms(facej)
            #             img_jittered.unsqueeze_(0)
            #             img = torch.cat([img, img_jittered])

                # forward pass
                    output = model(img)#.cuda())
                    # if jitter > 0:
                    #     output = torch.mean(output, 0)
                    score = F.sigmoid(output).item()
                    break
                    # coloridx = min(int(round(score*10)),9)
                    # draw = ImageDraw.Draw(pil_frame)
                    # #drawrect(draw, [(b[0], b[1]), (b[2], b[3])], outline=colors[coloridx].hex, width=5)
                    # #draw.text((b[0],b[3]), str(round(score,2)), fill=(255,255,255,128), font=font)
                    # drawrect(draw, [(x, y), (x+w, y+w)], outline=colors[coloridx].hex, width=5)
                    # draw.text((x,y+w), str(round(score,2)), fill=(255,255,255,128), font=font)
                    # print("Score1:{:.2f}".format(score))
                    # #print("Score:",  round(score, 3))
                    # send_ipc(hPipe, score)
            #     if save_text:
            #         f.write("%d,%f\n"%(frame_cnt,score))
            #
            # if not display_off:
            #     frame = np.asarray(frame) # convert PIL image back to opencv format for faster display
            #     frame = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
                    #cv2.imshow('',frame)
                    #cv2.imshow('',face)
                    # if vis:
                    #     outvid.write(frame)
                    key = cv2.waitKey(1) & 0xFF
                    if key == ord('q'):
                        break
        # else:
        #     break
            if score > 0.0:
                break
        send_ipc(hPipe, score)
    # if vis:
    #     outvid.release()
    # if save_text:
    #     f.close()
    cap.release()
    print ('DONE!')



MESSAGE = u"I am python codebase\n"
szPipename = r"\\.\\pipe\\ipc"


PIPE_ACCESS_DUPLEX = 0x3
PIPE_TYPE_MESSAGE = 0x4
PIPE_READMODE_MESSAGE = 0x2
PIPE_WAIT = 0
PIPE_UNLIMITED_INSTANCES = 255
BUFSIZE = 4096
NMPWAIT_USE_DEFAULT_WAIT = 0
INVALID_HANDLE_VALUE = -1
ERROR_PIPE_CONNECTED = 53

def ReadWrite_ClientPipe_Thread(hPipe):

    chBuf = create_string_buffer(BUFSIZE)
    cbRead = c_ulong(0)
    hPipe = win32file.CreateFile( szPipename, win32file.GENERIC_WRITE, 0, None, win32file.OPEN_EXISTING, 0, None )#
    if (hPipe == INVALID_HANDLE_VALUE):
        print("Error in creating Named Pipe")

    v = 0.0
    while 1:

        if True == True:#v != ipc_score
        #    v = ipc_score
            strv1 = str(v)
            strv2 = str(v)
            strv = strv1 + strv2
         #   print("ipc_score:", ipc_score)
            # fSuccess = windll.kernel32.ReadFile(hPipe, chBuf, BUFSIZE, byref(cbRead), None)
            # if ((fSuccess ==1) or (cbRead.value != 0)):
            #     print(chBuf.value)
            #     cbWritten = c_ulong(0)
            #     fSuccess = windll.kernel32.WriteFile(hPipe,c_char_p(MESSAGE),len(MESSAGE),byref(cbWritten),None)
            # else:
            #     break
            cbWritten = c_ulong(0)
            l = len(strv)
            print("Len:", l)
            fSuccess = windll.kernel32.WriteFile(c_ulonglong(hPipe), c_wchar_p(strv), c_ulong(l), byref(cbWritten), None)
            if ( (not fSuccess) or (len(strv) != cbWritten.value)):
                print("Could not reply to the client's request from the pipe")
                break
            else:
                print("Number of bytes written:", cbWritten.value)
            time.sleep(1)
    windll.kernel32.FlushFileBuffers(hPipe)
    windll.kernel32.DisconnectNamedPipe(hPipe)
    windll.kernel32.CloseHandle(hPipe)
    return 0

def IPC():
    THREADFUNC = CFUNCTYPE(c_int, c_int)
    thread_func = THREADFUNC(ReadWrite_ClientPipe_Thread)
    hPipe = windll.kernel32.CreateNamedPipeA(szPipename,PIPE_ACCESS_DUPLEX,PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT, PIPE_UNLIMITED_INSTANCES, BUFSIZE, BUFSIZE, NMPWAIT_USE_DEFAULT_WAIT,None)
    if (hPipe == INVALID_HANDLE_VALUE):
        print("Error in creating Named Pipe")
        return 0

    fConnected = windll.kernel32.ConnectNamedPipe(hPipe, None)
    if ((fConnected == 0) and (windll.kernel32.GetLastError() == ERROR_PIPE_CONNECTED)):
        fConnected = 1
    if (fConnected == 1):
        dwThreadId = c_ulong(0)
        hThread = windll.kernel32.CreateThread(None, 0, thread_func, hPipe, 0, byref(dwThreadId))
        if (hThread == -1):
            print("Create Thread failed")
            return 0
        else:
            windll.kernel32.CloseHandle(hThread)
    else:
        print("Could not connect to the Named Pipe")
        windll.kernel32.CloseHandle(hPipe)
    return 0

if __name__ == "__main__":
    #IPC()
    # THREADFUNC = CFUNCTYPE(c_int, c_int)
    # thread_func = THREADFUNC(ReadWrite_ClientPipe_Thread)
    # dwThreadId = c_ulong(0)
    # hThread = windll.kernel32.CreateThread(None, 0, thread_func, 0, 0, byref(dwThreadId))
    # if (hThread == -1):
    #     print("Create Thread failed")
    # else:
    #     windll.kernel32.CloseHandle(hThread)
#    while(1):
#        time.sleep(10)

    hPipe = win32file.CreateFile( szPipename, win32file.GENERIC_WRITE, 0, None, win32file.OPEN_EXISTING, 0, None )#
    if (hPipe == INVALID_HANDLE_VALUE):
        print("Error in creating Named Pipe")
    else:
        run(args.video, args.face, args.model_weight, args.jitter, args.save_vis, args.display_off, args.save_text, hPipe)
        windll.kernel32.FlushFileBuffers(hPipe)
        windll.kernel32.DisconnectNamedPipe(hPipe)
        windll.kernel32.CloseHandle(hPipe)


