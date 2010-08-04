/*
    SDL - Simple DirectMedia Layer
    Copyright (C) 2010 Eli Gottlieb

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

    Eli Gottlieb
    eligottlieb@gmail.com
*/

#include "SDL_win32shape.h"
#include "SDL_win32video.h"

SDL_WindowShaper* Win32_CreateShaper(SDL_Window * window) {
	int resized_properly;
	SDL_WindowShaper* result = (SDL_WindowShaper *)SDL_malloc(sizeof(SDL_WindowShaper));
	result->window = window;
	result->mode.mode = ShapeModeDefault;
	result->mode.parameters.binarizationCutoff = 1;
	result->usershownflag = 0;
	result->driverdata = (SDL_ShapeData*)SDL_malloc(sizeof(SDL_ShapeData));
	((SDL_ShapeData*)result->driverdata)->mask_tree = NULL;
	//Put some driver-data here.
	window->shaper = result;
	resized_properly = Win32_ResizeWindowShape(window);
	if (resized_properly != 0)
			return NULL;

	return result;
}

void CombineRectRegions(SDL_ShapeTree* node, void* closure) {
	HRGN* mask_region = (HRGN *)closure;
	if(node->kind == OpaqueShape) {
		HRGN temp_region = CreateRectRgn(node->data.shape.x,node->data.shape.y,node->data.shape.w,node->data.shape.h);
		CombineRgn(*mask_region,*mask_region,temp_region, RGN_OR);
		DeleteObject(temp_region);
	}
}

int Win32_SetWindowShape(SDL_WindowShaper *shaper,SDL_Surface *shape,SDL_WindowShapeMode *shapeMode) {
	SDL_ShapeData *data;
	HRGN mask_region;
    SDL_WindowData *windowdata;
    HWND hwnd;

	if (shaper == NULL || shape == NULL)
		return SDL_INVALID_SHAPE_ARGUMENT;
	if(shape->format->Amask == 0 && shapeMode->mode != ShapeModeColorKey || shape->w != shaper->window->w || shape->h != shaper->window->h)
		return SDL_INVALID_SHAPE_ARGUMENT;
	
	data = (SDL_ShapeData*)shaper->driverdata;
	if(data->mask_tree != NULL)
		SDL_FreeShapeTree(&data->mask_tree);
	data->mask_tree = SDL_CalculateShapeTree(*shapeMode,shape,SDL_FALSE);
	
	/*
	 * Start with empty region 
	 */
	mask_region = CreateRectRgn(0, 0, 0, 0);
	
	SDL_TraverseShapeTree(data->mask_tree,&CombineRectRegions,&mask_region);
	
	/*
	 * Set the new region mask for the window 
	 */
	windowdata=(SDL_WindowData *)(shaper->window->driverdata);
	hwnd = windowdata->hwnd;
	SetWindowRgn(hwnd, mask_region, TRUE);
	
	return 0;
}

int Win32_ResizeWindowShape(SDL_Window *window) {
	SDL_ShapeData* data;

	if (window == NULL)
		return -1;
	data = (SDL_ShapeData *)window->shaper->driverdata;
	if (data == NULL)
		return -1;
	
	if(data->mask_tree != NULL)
		SDL_FreeShapeTree(&data->mask_tree);
	
	window->shaper->usershownflag |= window->flags & SDL_WINDOW_SHOWN;
	
	return 0;
}