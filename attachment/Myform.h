#pragma once
#include "include\cmpvdeco.h"
#include "include\tchar.h"
#include "include\dshow.h"
#include "include\convert.h"
#define _USE_MATH_DEFINES
#include <math.h>
#using "mscorlib.dll"
#using "System.Drawing.dll"
#include <iostream>
#include "include\Windows.h"

struct ValueBlockStruct
{
	short x00, x01, x02, x03, x10, x11, x12, x13, x20, x21, x22, x23, x30, x31, x32, x33;
} typedef ValueBlocks;

namespace face
{
	using namespace System;
	using namespace System::ComponentModel;
	using namespace System::Collections;
	using namespace System::Windows::Forms;
	using namespace System::Data;
	using namespace System::Drawing;
	using namespace System::Runtime::InteropServices;

	WCHAR wpFileName[200];

	public ref class Form1 : public System::Windows::Forms::Form
	{
	public:
		Form1(void)
		{
			InitializeComponent();
		}
		~Form1(void)
		{}

	private: System::Windows::Forms::PictureBox ^  pictureBox;
	private: System::Windows::Forms::RichTextBox ^ textBox;
	private: System::Windows::Forms::Button ^  extractButton;
	private: System::Windows::Forms::Button ^  motionButton;
	private: System::Windows::Forms::Button ^  motionButtonP;
	//private: System::Windows::Forms::Button ^  motionButtonH;
	private: System::Windows::Forms::OpenFileDialog ^  openFileDialog;

	private:
		void InitializeComponent(void)
		{
			this->pictureBox = (gcnew System::Windows::Forms::PictureBox());
			pictureBox->Location = Point(50, 50);
			pictureBox->Size = System::Drawing::Size(400, 400);

			//file dialog
			this->openFileDialog = (gcnew System::Windows::Forms::OpenFileDialog());

			//button
			this->extractButton = (gcnew System::Windows::Forms::Button());
			this->extractButton->Location = System::Drawing::Point(145, 550);
			this->extractButton->Size = System::Drawing::Size(160, 50);
			this->extractButton->TabIndex = 3;
			this->extractButton->Text = L"Extract Picture";
			this->extractButton->Click += gcnew System::EventHandler(this, &Form1::extractButton_click);

			this->Controls->Add(this->pictureBox);
			this->Controls->Add(this->extractButton);


			this->AutoScaleBaseSize = System::Drawing::Size(8, 19);
			this->ClientSize = System::Drawing::Size(900, 650);
		}

	private:
		System::Void extractButton_click(System::Object ^  sender, System::EventArgs ^  e)
		{
			const HEADERDATA *pMpvHeaderData;
			char psFileName[200];
			CMpvDecoder MpvDecoder;
			WORD PicWidth, PicHeight, CrPicWidth, CrPicHeight, wYPicWidth, wYPicHeight;
			long lResult;
			long lastnum;
			COMPRESSEDPIC *pCompressedPic;

			if (this->openFileDialog->ShowDialog() == System::Windows::Forms::DialogResult::OK)
			{
				int i = this->openFileDialog->FileName->Length;
				for (int j = 0; j < i; j++)
				{
					psFileName[j] = this->openFileDialog->FileName->ToCharArray()[j];
				}
				for (int j = i; j < 200; j++)
				{
					psFileName[j] = 0;
				}
				if ((lResult = MpvDecoder.Initialize(psFileName)) != MDC_SUCCESS)
					return;
			}

			pMpvHeaderData = MpvDecoder.GetMpvHeaders();
			SEQUENCEHEADER sequencehead = pMpvHeaderData->SequenceHeader;
			wYPicWidth = sequencehead.wHorizontalSize / 16;
			wYPicHeight = sequencehead.wVerticalSize / 16;
			PicWidth = sequencehead.wHorizontalSize;
			PicHeight = sequencehead.wVerticalSize;
			int blocknum = wYPicHeight * wYPicWidth;

			pCompressedPic = MpvDecoder.PrepareCompressedPic();
			MpvDecoder.GetMaxFrameNum(&lastnum);

			lResult = MpvDecoder.GetCompressedPic();
			pMpvHeaderData = MpvDecoder.GetMpvHeaders();

			System::Drawing::Bitmap^ bitmap1 = gcnew System::Drawing::Bitmap((int)PicWidth, (int)PicHeight,System::Drawing::Imaging::PixelFormat::Format24bppRgb);
			System::Drawing::Rectangle^ rect1 = gcnew System::Drawing::Rectangle(0, 0, (int)PicWidth, (int)PicHeight);
			System::Drawing::Imaging::BitmapData^ bmData1 = bitmap1->LockBits(
				*rect1,
				System::Drawing::Imaging::ImageLockMode::ReadWrite,
				System::Drawing::Imaging::PixelFormat::Format24bppRgb);

			ColorSpaceConversions conv;
			int m_stride = bmData1->Stride;
			int m_scan0 = bmData1->Scan0.ToInt32();
			BYTE* p = (BYTE*)(void*)m_scan0;

			BYTE* pYBuff = new BYTE[wYPicWidth * 8 * wYPicHeight * 8 * 4];
			BYTE* pCbBuff = new BYTE[wYPicWidth * 8 * wYPicHeight * 8];
			BYTE* pCrBuff = new BYTE[wYPicWidth * 8 * wYPicHeight * 8];
			BYTE* pRGBBuff = new BYTE[wYPicWidth * 8 * wYPicHeight * 8 * 12];

			//reduction Y
			DCTBLOCK *Y = pCompressedPic->pshYDCTBlock;
			for (int j = 0; j < wYPicWidth*wYPicHeight * 4; j++)
			{
				//Which column takes the remainder
				int x = (j % (wYPicWidth * 2)) * 8;
				//Which line is the number of divisors
				int y = (j / (wYPicWidth * 2)) * 8;
				cal4m4Pixels(pYBuff, x, y, PicWidth, Y, j);
				std::cout << "block" << j << ": ";
				std::cout << "Y00:" << Y[j][0] <<" ";
				std::cout << "Y10:" << Y[j][8] <<" ";
				std::cout <<"Y20:"<< Y[j][16]<<" ";
				std::cout << "Y01:" << Y[j][1] << std::endl;
			}

			DCTBLOCK *Cb = pCompressedPic->pshCbDCTBlock;
			DCTBLOCK *Cr = pCompressedPic->pshCrDCTBlock;

			//Reduction CbCr
			for (int j = 0; j < wYPicWidth*wYPicHeight; j++)
			{
				int x = (j % wYPicWidth) * 8;
				int y = (j / wYPicWidth) * 8;
				cal4m4Pixels(pCbBuff, x, y, PicWidth / 2, Cb, j);
				cal4m4Pixels(pCrBuff, x, y, PicWidth / 2, Cr, j);
				std::cout << "block" << j<<": ";
				std::cout << "Cb00:" << Cb[j][0] << " ";
				std::cout << "Cb10:" << Cb[j][8] << " ";
				std::cout << "Cb20:" << Cb[j][16] <<" ";
				std::cout << "Cb01:" << Cb[j][1] <<" ";
				std::cout << "Cr00:" << Cr[j][0] <<" ";
				std::cout << "Cr10:" << Cr[j][8] <<" ";
				std::cout << "Cr20:" << Cr[j][16] <<" ";
				std::cout << "Cr01:" << Cr[j][1] << std::endl;
			}

			conv.YV12_to_RGB24(pYBuff, pCbBuff, pCrBuff, pRGBBuff, wYPicWidth * 16, wYPicHeight * 16);
			BYTE* q = pRGBBuff + wYPicWidth * 8 * 6 * (wYPicHeight * 8 * 2 - 1);

			for (int j = 0; j < wYPicHeight * 8 * 2; j++)
			{
				for (int k = 0; k < wYPicWidth * 8 * 2; k++)
				{
					p[0] = q[k * 3];
					p[1] = q[k * 3 + 1];
					p[2] = q[k * 3 + 2];
					p = p + 3;

				}
				q = q - wYPicWidth * 8 * 6;
			}

			bitmap1->UnlockBits(bmData1);

			this->pictureBox->Image = bitmap1;
			this->pictureBox->Update();

			MpvDecoder.FreeCompressedPic();
			delete[] pYBuff;
			delete[] pCbBuff;
			delete[] pCrBuff;
			delete[] pRGBBuff;
		}

		

		void cal2m2Pixels(Byte * pbuff, int x, int y, int num_per_row, DCTBLOCK * C, int blocknum)
		{
			int index = 0;
			short A00 = (short)((C[blocknum][0] + C[blocknum][8] + C[blocknum][1] + C[blocknum][9]) >> 3);
			short A01 = (short)((C[blocknum][0] + C[blocknum][8] - C[blocknum][1] - C[blocknum][9]) >> 3);
			short A10 = (short)((C[blocknum][0] - C[blocknum][8] + C[blocknum][1] - C[blocknum][9]) >> 3);
			short A11 = (short)((C[blocknum][0] - C[blocknum][8] - C[blocknum][1] + C[blocknum][9]) >> 3);
			for (int i = 0; i < 8; i++)
			{
				for (int j = 0; j < 8; j++)
				{
					index = x + i + (y + j) * num_per_row;
					switch (i)
					{
					case 0:
					case 1:
					case 2:
					case 3:
						switch (j)
						{
						case 0:
						case 1:
						case 2:
						case 3:
							pbuff[index] = (Byte)(A00 + 128);
							break;
						case 4:
						case 5:
						case 6:
						case 7: //x30
							pbuff[index] = (Byte)(A10 + 128);
							break;
						}
						break;
					case 4:
					case 5:
					case 6:
					case 7:
						switch (j)
						{
						case 0:
						case 1:
						case 2:
						case 3: //x13
							pbuff[index] = (Byte)(A01 + 128);
							break;
						case 4:
						case 5:
						case 6:
						case 7: //x33
							pbuff[index] = (Byte)(A11 + 128);
						}
						break;
					}
				}
			}

		}


		void cal4m4Pixels(Byte * pbuff, int x, int y, int num_per_row, DCTBLOCK * C, int blocknum)
		{
			int index = 0;
			ValueBlocks VB;
			calValues(C, blocknum, VB);
			for (int i = 0; i < 8; i++)
			{
				for (int j = 0; j < 8; j++)
				{
					index = x + i + (y + j) * num_per_row;
					if (i == 0 || i == 1)
					{
						if (j == 0 || j == 1)
						{
							pbuff[index] = VB.x00 + 128;
						}
						else if (j == 2 || j == 3)
						{
							pbuff[index] = VB.x10 + 128;
						}
						else if (j == 4 || j == 5)
						{
							pbuff[index] = VB.x20 + 128;
						}
						else if (j == 6 || j == 7)
						{
							pbuff[index] = VB.x30 + 128;
						}
					}
					else if (i ==2 || i == 3)
					{
						if (j == 0 || j == 1)
						{
							pbuff[index] = VB.x01 + 128;
						}
						else if (j == 2 || j == 3)
						{
							pbuff[index] = VB.x11 + 128;
						}
						else if (j == 4 || j == 5)
						{
							pbuff[index] = VB.x21 + 128;
						}
						else if (j == 6 || j == 7) {
							pbuff[index] = VB.x31 + 128;
						}
					}
					else if (i == 4 || i == 5)
					{
						if (j == 0 || j == 1) {
							pbuff[index] = VB.x02 + 128;
						}
						else if (j == 2 || j == 3) {
							pbuff[index] = VB.x12 + 128;
						}
						else if (j == 4 || j == 5) {
							pbuff[index] = VB.x22 + 128;
						}
						else if (j == 6 || j == 7) {
							pbuff[index] = VB.x32 + 128;
						}
					}
					else if(i == 6 || i ==7)
					{
						if (j == 0 || j == 1) {
							pbuff[index] = VB.x03 + 128;
						}
						else if (j == 2 || j == 3) {
							pbuff[index] = VB.x13 + 128;
						}
						else if (j == 4 || j == 5) {
							pbuff[index] = VB.x23 + 128;
						}
						else if(j == 6 || j ==7){
							pbuff[index] = VB.x33 + 128;
						}
					}
				}
			}
		}


		void calValues(DCTBLOCK * C, int j, ValueBlocks& VB) {
			//std::cout << C[j][0];
			short pA1 = C[j][0] + C[j][16];
			short pA2 = C[j][0] - C[j][16];
			short pA3 = C[j][2] + C[j][18];
			short pA4 = C[j][2] - C[j][18];

			short pB1 = C[j][1] + C[j][17];
			short pB2 = C[j][1] - C[j][17];
			short pB3 = C[j][8] + C[j][10];
			short pB4 = C[j][8] - C[j][10];



			//9 Values
			VB.x00 = ((pA1 + pA3 + pB3 + pB1 + C[j][9]) >> 3) + ((pB3 + pB1 + C[j][9]) >> 5);
			VB.x10 = ((pA2 + pA4 + pB2) >> 3) + (pB2 >> 5) + ((pB3 + C[j][9]) >> 4);
			VB.x20 = ((pA2 + pA4 + pB2) >> 3) + (pB2 >> 5) - ((pB3 + C[j][9]) >> 4);
			VB.x30 = ((pA1 + pA3) >> 3) - ((pB3 - pB1 + C[j][9]) >> 4);
			VB.x01 = ((pA1 - pA3 + pB3) >> 3) + (pB3 >> 5) + ((pB1 + C[j][9]) >> 4);
			VB.x11 = ((pA2 - pA4) >> 3) + ((pB4 + pB2) >> 4);
			VB.x21 = ((pA2 - pA4) >> 3) - ((pB4 - pB2) >> 4);
			VB.x31 = ((pA1 - pA3 - pB4) >> 3) - (pB4 >> 5) + ((pB1 + C[j][9]) >> 4);
			VB.x02 = ((pA1 - pA3 + pB4) >> 3) + (pB4 >> 5) - ((pB1 + C[j][9]) >> 4);
			VB.x12 = ((pA2 - pA4) >> 3) + ((pB4 - pB2) >> 4);
			VB.x22 = ((pA2 - pA4) >> 3) - ((pB4 + pB2) >> 4);
			VB.x32 = ((pA1 - pA3 - pB4) >> 3) - (pB4 >> 5) - ((pB1 - C[j][9]) >> 4);
			VB.x03 = ((pA1 + pA3 + pB4 - pB1 - C[j][9]) >> 3) + ((pB4 - pB1 - C[j][9]) >> 5);
			VB.x13 = ((pA2 + pA4 - pB2) >> 3) - (pB2 >> 5) + ((pB3 - C[j][9]) >> 4);
			VB.x23 = ((pA2 + pA4 - pB2) >> 3) - (pB2 >> 5) - ((pB3 - C[j][9]) >> 4);
			VB.x33 = ((pA1 + pA3 - pB4 - pB1 + C[j][9]) >> 3) - ((pB4 + pB1 - C[j][9]) >> 5);

			//add 7 more Values
			short p00, p10, p20, p30, p01, p11, p21, p31, p02, p12, p22, p32, p03, p13, p23, p33;
			p00 = ((C[j][24] + C[j][26] + C[j][3] + C[j][19] + C[j][27]) >> 4) + ((C[j][25] + C[j][11]) >> 3);
			p10 = -((C[j][24] + C[j][26] + C[j][27]) >> 3) + ((C[j][3] - C[j][19]) >> 4) - (C[j][25] >> 2) + (C[j][11] >> 5);
			p20 = ((C[j][24] + C[j][26] + C[j][27]) >> 3) + ((C[j][3] - C[j][19]) >> 4) + (C[j][25] >> 2) - (C[j][11] >> 5);
			p30 = -((C[j][24] + C[j][26] - C[j][3] - C[j][19] + C[j][27]) >> 4) + ((C[j][25] + C[j][11]) >> 3);
			p01 = ((C[j][24] - C[j][26]) >> 4) - ((C[j][3] + C[j][19] + C[j][27]) >> 3) + (C[j][25] >> 5) - (C[j][11] >> 2);
			p11 = -((C[j][24] - C[j][26] + C[j][3] - C[j][19]) >> 3) - ((C[j][25] + C[j][11]) >> 4) + (C[j][27] >> 2);
			p21 = ((C[j][24] - C[j][26] - C[j][3] + C[j][19]) >> 3) + ((C[j][25] + C[j][11]) >> 4) - (C[j][27] >> 2);
			p31 = -((C[j][24] - C[j][26]) >> 4) - ((C[j][3] + C[j][19] - C[j][27]) >> 3) - (C[j][25] >> 5) + (C[j][11] >> 2);
			p02 = ((C[j][24] - C[j][26]) >> 4) + ((C[j][3] + C[j][19] + C[j][27]) >> 3) - (C[j][25] >> 5) + (C[j][11] >> 2);
			p12 = -((C[j][24] - C[j][26] - C[j][3] + C[j][19]) >> 3) + ((C[j][25] + C[j][11]) >> 4) - (C[j][27] >> 2);
			p22 = ((C[j][24] - C[j][26] + C[j][3] - C[j][19]) >> 3) - ((C[j][25] + C[j][11]) >> 4) + (C[j][27] >> 2);
			p32 = -((C[j][24] - C[j][26]) >> 4) + ((C[j][3] + C[j][19] - C[j][27]) >> 3) + (C[j][25] >> 5) - (C[j][11] >> 2);
			p03 = ((C[j][24] + C[j][26] - C[j][3] - C[j][19] - C[j][27]) >> 4) + ((C[j][25] + C[j][11]) >> 3);
			p13 = -((C[j][24] + C[j][26] - C[j][27]) >> 3) - ((C[j][3] - C[j][19]) >> 4) + (C[j][25] >> 2) - (C[j][11] >> 5);
			p23 = ((C[j][24] + C[j][26] - C[j][27]) >> 3) - ((C[j][3] - C[j][19]) >> 4) - (C[j][25] >> 2) + (C[j][11] >> 5);
			p33 = -((C[j][24] + C[j][26] + C[j][3] + C[j][19] -C[j][27]) >> 4) + ((C[j][25] + C[j][11]) >> 3);

			std::cout << "!x00:" << VB.x00 << "x10:" << VB.x10 << "x20:" << VB.x20 << "x30:" << VB.x30 << "\nx01:" << VB.x01 << "x11:" << VB.x11 << "x21:" << VB.x21 << "x31:" << VB.x31 << "\nx02:" << VB.x02 << "x12:" << VB.x12 << "x22:" << VB.x22 << "x32:" << VB.x32 << "\nx03:" << VB.x03 << "x13:" << VB.x13 << "x23:" << VB.x23 << "x33:" << VB.x33 << std::endl;

			VB.x00 = VB.x00 + p00;
			VB.x10 = VB.x10 + p10;
			VB.x20 = VB.x20 + p20;
			VB.x30 = VB.x30 + p30;
			VB.x01 = VB.x01 + p01;
			VB.x11 = VB.x11 + p11;
			VB.x21 = VB.x21 + p21;
			VB.x31 = VB.x31 + p31;
			VB.x02 = VB.x02 + p02;
			VB.x12 = VB.x12 + p12;
			VB.x22 = VB.x22 + p22;
			VB.x32 = VB.x32 + p32;
			VB.x03 = VB.x03 + p03;
			VB.x13 = VB.x13 + p13;
			VB.x23 = VB.x23 + p23;
			VB.x33 = VB.x33 + p33;

			std::cout <<"x00:"<< VB.x00 << "x10:" << VB.x10 << "x20:" << VB.x20 << "x30:" << VB.x30 << "\nx01:" << VB.x01 << "x11:" << VB.x11 << "x21:" << VB.x21 << "x31:" << VB.x31 << "\nx02:" << VB.x02 << "x12:" << VB.x12 << "x22:" << VB.x22 << "x32:" << VB.x32 << "\nx03:" << VB.x03 << "x13:" << VB.x13 << "x23:" << VB.x23 << "x33:" << VB.x33 << std::endl;

		}
	};
}

