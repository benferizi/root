// @(#)root/graf:$Id$
// Author: Otto Schaile   20/11/99

/*************************************************************************
 * Copyright (C) 1995-2000, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

/** \class TCurlyArc
\ingroup BasicGraphics

Implements curly or wavy arcs used to draw Feynman diagrams.

Amplitudes and wavelengths may be specified in the constructors,
via commands or interactively from popup menus.
The class make use of TCurlyLine by inheritance, ExecuteEvent methods
are highly inspired from the methods used in TPolyLine and TArc.
The picture below has been generated by the tutorial feynman.

Begin_Macro(source)
../../../tutorials/graphics/feynman.C
End_Macro
*/

#include <iostream>
#include "TCurlyArc.h"
#include "TROOT.h"
#include "TVirtualPad.h"
#include "TVirtualX.h"
#include "TMath.h"
#include "TPoint.h"

Double_t TCurlyArc::fgDefaultWaveLength = 0.02;
Double_t TCurlyArc::fgDefaultAmplitude  = 0.01;
Bool_t   TCurlyArc::fgDefaultIsCurly    = kTRUE;

ClassImp(TCurlyArc);

////////////////////////////////////////////////////////////////////////////////
/// Default constructor

TCurlyArc::TCurlyArc()
{
   fR1     = 0.;
   fPhimin = 0.;
   fPhimax = 0.;
   fTheta  = 0.;
}

////////////////////////////////////////////////////////////////////////////////
/// Create a new TCurlyArc with center (x1, y1) and radius rad.
/// The wavelength and amplitude are given in percent of the line length
/// phimin and phimax are given in degrees.

TCurlyArc::TCurlyArc(Double_t x1, Double_t y1,
                   Double_t rad, Double_t phimin, Double_t phimax,
                   Double_t wl, Double_t amp)
         : fR1(rad), fPhimin(phimin),fPhimax(phimax)
{
   fX1         = x1;
   fY1         = y1;
   fIsCurly    = fgDefaultIsCurly;
   fAmplitude  = amp;
   fWaveLength = wl;
   fTheta      = 0;
   Build();
}

////////////////////////////////////////////////////////////////////////////////
/// Create a curly (Gluon) or wavy (Gamma) arc.

void TCurlyArc::Build()
{
   Double_t pixeltoX = 1;
   Double_t pixeltoY = 1;
   Double_t rPix = fR1;
   if (gPad) {
      Double_t ww = (Double_t)gPad->GetWw();
      Double_t wh = (Double_t)gPad->GetWh();
      Double_t pxrange = gPad->GetAbsWNDC()*ww;
      Double_t pyrange = - gPad->GetAbsHNDC()*wh;
      Double_t xrange  = gPad->GetX2() - gPad->GetX1();
      Double_t yrange  = gPad->GetY2() - gPad->GetY1();
      pixeltoX  = xrange / pxrange;
      pixeltoY  = yrange/pyrange;
      rPix = fR1 / pixeltoX;
   }
   Double_t dang = fPhimax - fPhimin;
   if (dang < 0) dang += 360;
   Double_t length = TMath::Pi() * fR1 * dang/180;
   Double_t x1sav = fX1;
   Double_t y1sav = fY1;
   fX1 = fY1 = 0;
   fX2 = length;
   fY2 = 0;
   TCurlyLine::Build();
   fX1 = x1sav;
   fY1 = y1sav;
   Double_t *xv= GetX();
   Double_t *yv= GetY();
   Double_t xx, yy, angle;
   for(Int_t i = 0; i < fNsteps; i++){
      angle = xv[i] / rPix + fPhimin * TMath::Pi()/180;
      xx    = (yv[i] + rPix) * cos(angle);
      yy    = (yv[i] + rPix) * sin(angle);
      xx *= pixeltoX;
      yy *= TMath::Abs(pixeltoY);
      xv[i] = xx + fX1;
      yv[i] = yy + fY1;
   }
   if (gPad) gPad->Modified();
}

////////////////////////////////////////////////////////////////////////////////
/// Compute distance from point px,py to an arc.
///
///  Compute the closest distance of approach from point px,py to this arc.
///  The distance is computed in pixels units.

Int_t TCurlyArc::DistancetoPrimitive(Int_t px, Int_t py)
{
   if (!gPad) return 9999;
   // Compute distance of point to center of arc
   Int_t pxc    = gPad->XtoAbsPixel(fX1);
   Int_t pyc    = gPad->YtoAbsPixel(fY1);
   Double_t dist = TMath::Sqrt(Long64_t(pxc-px)*(pxc-px)+Long64_t(pyc-py)*(pyc-py));
   Double_t cosa = (px - pxc)/dist;
   Double_t sina = (pyc - py)/dist;
   Double_t phi  = TMath::ATan2(sina,cosa);
   if (phi < 0) phi += 2 * TMath::Pi();
   phi = phi * 180 / TMath::Pi();
   if (fPhimax > fPhimin){
      if (phi < fPhimin || phi > fPhimax) return 9999;
   } else {
      if (phi > fPhimin && phi < fPhimax) return 9999;
   }
   Int_t pxr = gPad->XtoPixel(fR1)- gPad->XtoPixel(0);
   Double_t distr = TMath::Abs(dist-pxr);
   return Int_t(distr);
}

////////////////////////////////////////////////////////////////////////////////
/// Execute action corresponding to one event.
///
/// This member function is called when a TCurlyArc is clicked with the locator
///
/// If Left button clicked on one of the line end points, this point
/// follows the cursor until button is released.
///
/// if Middle button clicked, the line is moved parallel to itself
/// until the button is released.

void TCurlyArc::ExecuteEvent(Int_t event, Int_t px, Int_t py)
{
   if (!gPad) return;

   Int_t kMaxDiff = 10;
   const Int_t np = 10;
   const Double_t pi = TMath::Pi();
   static Int_t x[np+3], y[np+3];
   static Int_t px1,py1,npe,r1;
   static Int_t pxold, pyold;
   Int_t i, dpx, dpy;
   Double_t angle,dx,dy,dphi,rLx,rRx;
   Double_t  phi0;
   static Bool_t pTop, pL, pR, pBot, pINSIDE;
   static Int_t pTx,pTy,pLx,pLy,pRx,pRy,pBx,pBy;

   Bool_t opaque  = gPad->OpaqueMoving();

   switch (event) {

   case kArrowKeyPress:
   case kButton1Down:
      if (!opaque) {
         gVirtualX->SetLineColor(-1);
         TAttLine::Modify();
         dphi = (fPhimax-fPhimin) * pi / 180;
         if (dphi<0) dphi += 2 * pi;
         dphi /= np;
         phi0 = fPhimin * pi / 180;
         for (i=0;i<=np;i++) {
            angle = Double_t(i)*dphi + phi0;
            dx    = fR1*TMath::Cos(angle);
            dy    = fR1*TMath::Sin(angle);
            Int_t rpixY = gPad->XtoAbsPixel(dy) - gPad->XtoAbsPixel(0);
            x[i]  = gPad->XtoAbsPixel(fX1 + dx);
            y[i]  = gPad->YtoAbsPixel(fY1) + rpixY;
         }
         if (fPhimax-fPhimin >= 360 ) {
            x[np+1] = x[0];
            y[np+1] = y[0];
            npe = np;
         } else {
            x[np+1]   = gPad->XtoAbsPixel(fX1);
            y[np+1]   = gPad->YtoAbsPixel(fY1);
            x[np+2] = x[0];
            y[np+2] = y[0];
            npe = np + 2;
         }
      }
      px1 = gPad->XtoAbsPixel(fX1);
      py1 = gPad->YtoAbsPixel(fY1);
      pTx = pBx = px1;
      pLy = pRy = py1;
      pLx = gPad->XtoAbsPixel(-fR1+fX1);
      pRx = gPad->XtoAbsPixel( fR1+fX1);
      r1 = TMath::Abs(pLx-pRx)/2;
      // a circle in pixels, radius measured along X
      pTy = gPad->YtoAbsPixel(fY1) + r1;
      pBy = gPad->YtoAbsPixel(fY1) - r1;

      if (!opaque) {
         gVirtualX->DrawLine(pRx+4, py1+4, pRx-4, py1+4);
         gVirtualX->DrawLine(pRx-4, py1+4, pRx-4, py1-4);
         gVirtualX->DrawLine(pRx-4, py1-4, pRx+4, py1-4);
         gVirtualX->DrawLine(pRx+4, py1-4, pRx+4, py1+4);
         gVirtualX->DrawLine(pLx+4, py1+4, pLx-4, py1+4);
         gVirtualX->DrawLine(pLx-4, py1+4, pLx-4, py1-4);
         gVirtualX->DrawLine(pLx-4, py1-4, pLx+4, py1-4);
         gVirtualX->DrawLine(pLx+4, py1-4, pLx+4, py1+4);
         gVirtualX->DrawLine(px1+4, pBy+4, px1-4, pBy+4);
         gVirtualX->DrawLine(px1-4, pBy+4, px1-4, pBy-4);
         gVirtualX->DrawLine(px1-4, pBy-4, px1+4, pBy-4);
         gVirtualX->DrawLine(px1+4, pBy-4, px1+4, pBy+4);
         gVirtualX->DrawLine(px1+4, pTy+4, px1-4, pTy+4);
         gVirtualX->DrawLine(px1-4, pTy+4, px1-4, pTy-4);
         gVirtualX->DrawLine(px1-4, pTy-4, px1+4, pTy-4);
         gVirtualX->DrawLine(px1+4, pTy-4, px1+4, pTy+4);
      }
      // No break !!!

   case kMouseMotion:
      px1 = gPad->XtoAbsPixel(fX1);
      py1 = gPad->YtoAbsPixel(fY1);
      pTx = pBx = px1;
      pLy = pRy = py1;
      pLx = gPad->XtoAbsPixel(-fR1+fX1);
      pRx = gPad->XtoAbsPixel( fR1+fX1);

      pTy = gPad->YtoAbsPixel(fY1) + TMath::Abs(pLx-pRx)/2;
      pBy = gPad->YtoAbsPixel(fY1) - TMath::Abs(pLx-pRx)/2;

      pTop = pL = pR = pBot = pINSIDE = kFALSE;
      if ((TMath::Abs(px - pTx) < kMaxDiff) &&
          (TMath::Abs(py - pTy) < kMaxDiff)) {             // top edge
         pTop = kTRUE;
         gPad->SetCursor(kTopSide);
      }
      else
      if ((TMath::Abs(px - pBx) < kMaxDiff) &&
          (TMath::Abs(py - pBy) < kMaxDiff)) {             // bottom edge
         pBot = kTRUE;
         gPad->SetCursor(kBottomSide);
      }
      else
      if ((TMath::Abs(py - pLy) < kMaxDiff) &&
          (TMath::Abs(px - pLx) < kMaxDiff)) {             // left edge
         pL = kTRUE;
         gPad->SetCursor(kLeftSide);
      }
      else
      if ((TMath::Abs(py - pRy) < kMaxDiff) &&
          (TMath::Abs(px - pRx) < kMaxDiff)) {             // right edge
         pR = kTRUE;
         gPad->SetCursor(kRightSide);
      }
      else {pINSIDE= kTRUE; gPad->SetCursor(kMove); }
      pxold = px;  pyold = py;

      break;

   case kArrowKeyRelease:
   case kButton1Motion:
      if (!opaque) {
         gVirtualX->DrawLine(pRx+4, py1+4, pRx-4, py1+4);
         gVirtualX->DrawLine(pRx-4, py1+4, pRx-4, py1-4);
         gVirtualX->DrawLine(pRx-4, py1-4, pRx+4, py1-4);
         gVirtualX->DrawLine(pRx+4, py1-4, pRx+4, py1+4);
         gVirtualX->DrawLine(pLx+4, py1+4, pLx-4, py1+4);
         gVirtualX->DrawLine(pLx-4, py1+4, pLx-4, py1-4);
         gVirtualX->DrawLine(pLx-4, py1-4, pLx+4, py1-4);
         gVirtualX->DrawLine(pLx+4, py1-4, pLx+4, py1+4);
         gVirtualX->DrawLine(px1+4, pBy+4, px1-4, pBy+4);
         gVirtualX->DrawLine(px1-4, pBy+4, px1-4, pBy-4);
         gVirtualX->DrawLine(px1-4, pBy-4, px1+4, pBy-4);
         gVirtualX->DrawLine(px1+4, pBy-4, px1+4, pBy+4);
         gVirtualX->DrawLine(px1+4, pTy+4, px1-4, pTy+4);
         gVirtualX->DrawLine(px1-4, pTy+4, px1-4, pTy-4);
         gVirtualX->DrawLine(px1-4, pTy-4, px1+4, pTy-4);
         gVirtualX->DrawLine(px1+4, pTy-4, px1+4, pTy+4);
         for (i=0;i<npe;i++) gVirtualX->DrawLine(x[i], y[i], x[i+1], y[i+1]);
      }
      if (pTop) {
         r1 += (py - pyold);
      }
      if (pBot) {
         r1 -= (py - pyold);
      }
      if (pL) {
         r1 -= (px - pxold);
      }
      if (pR) {
         r1 += (px - pxold);
      }
      if (pTop || pBot || pL || pR) {
         if (!opaque) {
            gVirtualX->SetLineColor(-1);
            TAttLine::Modify();
            dphi = (fPhimax-fPhimin) * pi / 180;
            if (dphi<0) dphi += 2 * pi;
            dphi /= np;
            phi0 = fPhimin * pi / 180;
            Double_t ur1 = r1;
            Int_t pX1   = gPad->XtoAbsPixel(fX1);
            Int_t pY1   = gPad->YtoAbsPixel(fY1);
            for (i=0;i<=np;i++) {
               angle = Double_t(i)*dphi + phi0;
               dx    = ur1 * TMath::Cos(angle);
               dy    = ur1 * TMath::Sin(angle);
               x[i]  = pX1 + (Int_t)dx;
               y[i]  = pY1 + (Int_t)dy;
            }
            if (fPhimax-fPhimin >= 360 ) {
               x[np+1] = x[0];
               y[np+1] = y[0];
               npe = np;
            } else {
               x[np+1]   = pX1;
               y[np+1]   = pY1;
               x[np+2] = x[0];
               y[np+2] = y[0];
               npe = np + 2;
            }
            for (i=0;i<npe;i++) {
               gVirtualX->DrawLine(x[i], y[i], x[i+1], y[i+1]);
            }
         }
         else {
            this->SetStartPoint(gPad->AbsPixeltoX(px1), gPad->AbsPixeltoY(py1));
            this->SetRadius(TMath::Abs(gPad->AbsPixeltoX(px1-r1)-gPad->AbsPixeltoX(px1+r1))/2);
            if (pTop) gPad->ShowGuidelines(this, event, 't', true);
            if (pBot) gPad->ShowGuidelines(this, event, 'b', true);
            if (pL) gPad->ShowGuidelines(this, event, 'l', true);
            if (pR) gPad->ShowGuidelines(this, event, 'r', true);
            gPad->Modified(kTRUE);
            gPad->Update();
         }
      }
      if (pINSIDE) {
         dpx  = px-pxold;  dpy = py-pyold;
         px1 += dpx; py1 += dpy;
         if (!opaque) {
            for (i=0;i<=npe;i++) { x[i] += dpx; y[i] += dpy;}
            for (i=0;i<npe;i++) gVirtualX->DrawLine(x[i], y[i], x[i+1], y[i+1]);
         } else {
            this->SetStartPoint(gPad->AbsPixeltoX(px1), gPad->AbsPixeltoY(py1));
            gPad->ShowGuidelines(this, event, 'i', true);
            gPad->Modified(kTRUE);
            gPad->Update();
         }
      }
      pTx = pBx = px1;
      pRx = px1+r1;
      pLx = px1-r1;
      pRy = pLy = py1;
      pTy = py1-r1;
      pBy = py1+r1;
      if (!opaque) {
         gVirtualX->DrawLine(pRx+4, py1+4, pRx-4, py1+4);
         gVirtualX->DrawLine(pRx-4, py1+4, pRx-4, py1-4);
         gVirtualX->DrawLine(pRx-4, py1-4, pRx+4, py1-4);
         gVirtualX->DrawLine(pRx+4, py1-4, pRx+4, py1+4);
         gVirtualX->DrawLine(pLx+4, py1+4, pLx-4, py1+4);
         gVirtualX->DrawLine(pLx-4, py1+4, pLx-4, py1-4);
         gVirtualX->DrawLine(pLx-4, py1-4, pLx+4, py1-4);
         gVirtualX->DrawLine(pLx+4, py1-4, pLx+4, py1+4);
         gVirtualX->DrawLine(px1+4, pBy+4, px1-4, pBy+4);
         gVirtualX->DrawLine(px1-4, pBy+4, px1-4, pBy-4);
         gVirtualX->DrawLine(px1-4, pBy-4, px1+4, pBy-4);
         gVirtualX->DrawLine(px1+4, pBy-4, px1+4, pBy+4);
         gVirtualX->DrawLine(px1+4, pTy+4, px1-4, pTy+4);
         gVirtualX->DrawLine(px1-4, pTy+4, px1-4, pTy-4);
         gVirtualX->DrawLine(px1-4, pTy-4, px1+4, pTy-4);
         gVirtualX->DrawLine(px1+4, pTy-4, px1+4, pTy+4);
      }
      pxold = px;
      pyold = py;
      break;

   case kButton1Up:
      if (opaque) {
         gPad->ShowGuidelines(this, event);
      } else {
         fX1 = gPad->AbsPixeltoX(px1);
         fY1 = gPad->AbsPixeltoY(py1);
         rLx = gPad->AbsPixeltoX(px1+r1);
         rRx = gPad->AbsPixeltoX(px1-r1);
         fR1 = TMath::Abs(rRx-rLx)/2;
      }
      Build();
      gPad->Modified(kTRUE);
      if (!opaque) gVirtualX->SetLineColor(-1);
   }
}

////////////////////////////////////////////////////////////////////////////////
/// Save primitive as a C++ statement(s) on output stream out

void TCurlyArc::SavePrimitive(std::ostream &out, Option_t * /*= ""*/)
{
   if (gROOT->ClassSaved(TCurlyArc::Class())) {
      out<<"   ";
   } else {
      out<<"   TCurlyArc *";
   }
   out<<"curlyarc = new TCurlyArc("
     <<fX1<<","<<fY1<<","<<fR1<<","<<fPhimin<<","<<fPhimax<<","
      <<fWaveLength<<","<<fAmplitude<<");"<<std::endl;
   if (!fIsCurly) {
      out<<"   curlyarc->SetWavy();"<<std::endl;
   }
   SaveLineAttributes(out,"curlyarc",1,1,1);
   out<<"   curlyarc->Draw();"<<std::endl;
}

////////////////////////////////////////////////////////////////////////////////
/// Set Curly Arc center.

void TCurlyArc::SetCenter(Double_t x, Double_t y)
{
   fX1 = x;
   fY1 = y;
   Build();
}

////////////////////////////////////////////////////////////////////////////////
/// Set Curly Arc radius.

void TCurlyArc::SetRadius(Double_t x)
{
   fR1 = x;
   Build();
}

////////////////////////////////////////////////////////////////////////////////
/// Set Curly Arc minimum Phi.

void TCurlyArc::SetPhimin(Double_t x)
{
   fPhimin = x;
   Build();
}

////////////////////////////////////////////////////////////////////////////////
/// Set Curly Arc maximum Phi.

void TCurlyArc::SetPhimax(Double_t x)
{
   fPhimax = x;
   Build();
}

////////////////////////////////////////////////////////////////////////////////
/// Set default wave length.

void TCurlyArc::SetDefaultWaveLength(Double_t WaveLength)
{
   fgDefaultWaveLength = WaveLength;
}

////////////////////////////////////////////////////////////////////////////////
/// Set default wave amplitude.

void TCurlyArc::SetDefaultAmplitude(Double_t Amplitude)
{
   fgDefaultAmplitude = Amplitude ;
}

////////////////////////////////////////////////////////////////////////////////
/// Set default "IsCurly".

void TCurlyArc::SetDefaultIsCurly(Bool_t IsCurly)
{
   fgDefaultIsCurly = IsCurly;
}

////////////////////////////////////////////////////////////////////////////////
/// Get default wave length.

Double_t TCurlyArc::GetDefaultWaveLength()
{
   return fgDefaultWaveLength;
}

////////////////////////////////////////////////////////////////////////////////
/// Get default wave amplitude.

Double_t TCurlyArc::GetDefaultAmplitude()
{
   return fgDefaultAmplitude;
}

////////////////////////////////////////////////////////////////////////////////
/// Get default "IsCurly".

Bool_t TCurlyArc::GetDefaultIsCurly()
{
   return fgDefaultIsCurly;
}

////////////////////////////////////////////////////////////////////////////////
/// Return the bounding Box of the Line

Rectangle_t TCurlyArc::GetBBox()
{
   Rectangle_t BBox{0,0,0,0};
   if (!gPad) return BBox;

   Double_t R2 = fR1 * TMath::Abs(gPad->GetY2()-gPad->GetY1())/TMath::Abs(gPad->GetX2()-gPad->GetX1());

   BBox.fX = gPad->XtoPixel(fX1-fR1);
   BBox.fY = gPad->YtoPixel(fY1+R2);
   BBox.fWidth = gPad->XtoPixel(fX1+fR1)-gPad->XtoPixel(fX1-fR1);
   BBox.fHeight = gPad->YtoPixel(fY1-R2)-gPad->YtoPixel(fY1+R2);
   return (BBox);
}

////////////////////////////////////////////////////////////////////////////////
/// Return the center of the BoundingBox as TPoint in pixels

TPoint TCurlyArc::GetBBoxCenter()
{
   TPoint p(0,0);
   if (!gPad) return (p);
   p.SetX(gPad->XtoPixel(fX1));
   p.SetY(gPad->YtoPixel(fY1));
   return(p);
}

////////////////////////////////////////////////////////////////////////////////
/// Set center of the BoundingBox

void TCurlyArc::SetBBoxCenter(const TPoint &p)
{
   if (!gPad) return;
   fX1 = gPad->PixeltoX(p.GetX());
   fY1 = gPad->PixeltoY(p.GetY()-gPad->VtoPixel(0));
   Build();
}

////////////////////////////////////////////////////////////////////////////////
/// Set X coordinate of the center of the BoundingBox

void TCurlyArc::SetBBoxCenterX(const Int_t x)
{
   if (!gPad) return;
   fX1 = gPad->PixeltoX(x);
   Build();
}

////////////////////////////////////////////////////////////////////////////////
/// Set Y coordinate of the center of the BoundingBox

void TCurlyArc::SetBBoxCenterY(const Int_t y)
{
   if (!gPad) return;
   fY1 = gPad->PixeltoY(y-gPad->VtoPixel(0));
   Build();
}

////////////////////////////////////////////////////////////////////////////////
/// Set left hand side of BoundingBox to a value
/// (resize in x direction on left)

void TCurlyArc::SetBBoxX1(const Int_t x)
{
   if (!gPad) return;
   Double_t x1 = gPad->PixeltoX(x);
   if (x1>fX1+fR1) return;

   fR1 = (fX1+fR1-x1)*0.5;
   fX1 = x1 + fR1;
}

////////////////////////////////////////////////////////////////////////////////
/// Set right hand side of BoundingBox to a value
/// (resize in x direction on right)

void TCurlyArc::SetBBoxX2(const Int_t x)
{
   if (!gPad) return;
   Double_t x2 = gPad->PixeltoX(x);
   if (x2<fX1-fR1) return;

   fR1 = (x2-fX1+fR1)*0.5;
   fX1 = x2-fR1;
}

////////////////////////////////////////////////////////////////////////////////
/// Set top of BoundingBox to a value (resize in y direction on top)

void TCurlyArc::SetBBoxY1(const Int_t y)
{
   if (!gPad) return;
   Double_t R2 = fR1 * TMath::Abs(gPad->GetY2()-gPad->GetY1())/TMath::Abs(gPad->GetX2()-gPad->GetX1());

   Double_t y1 = gPad->PixeltoY(y-gPad->VtoPixel(0));
   if (y1<fY1-R2) return;

   fR1 = (y1-fY1+R2)*0.5 / (TMath::Abs(gPad->GetY2()-gPad->GetY1())/TMath::Abs(gPad->GetX2()-gPad->GetX1()));
   fY1 = y1-R2;
}

////////////////////////////////////////////////////////////////////////////////
/// Set bottom of BoundingBox to a value
/// (resize in y direction on bottom)

void TCurlyArc::SetBBoxY2(const Int_t y)
{
   if (!gPad) return;
   Double_t R2 = fR1 * TMath::Abs(gPad->GetY2()-gPad->GetY1())/TMath::Abs(gPad->GetX2()-gPad->GetX1());

   Double_t y2 = gPad->PixeltoY(y-gPad->VtoPixel(0));

   if (y2>fY1+R2) return;

   fR1 = (fY1+R2-y2)*0.5 / (TMath::Abs(gPad->GetY2()-gPad->GetY1())/TMath::Abs(gPad->GetX2()-gPad->GetX1()));
   fY1 = y2+R2;
}
