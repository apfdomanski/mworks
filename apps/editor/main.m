//
//  main.m
//  NewEditor
//
//  Created by David Cox on 11/1/07.
//  Copyright __MyCompanyName__ 2007 . All rights reserved.
//

#import <Cocoa/Cocoa.h>

int main(int argc, char *argv[])
{
  @try{
    return NSApplicationMain(argc, (const char **) argv);
  } @catch (NSException *e) {
    NSLog(@"%@", [e reason]);
  }
  
}
