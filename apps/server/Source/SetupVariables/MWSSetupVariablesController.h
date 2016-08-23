//
//  MWSSetupVariablesController.h
//  MWServer
//
//  Created by Christopher Stawarz on 6/4/14.
//
//

#import <Foundation/Foundation.h>


@interface MWSSetupVariablesController : NSObject

@property(nonatomic, copy) NSString *serverName;
@property(weak, nonatomic, readonly) NSArray *availableDisplays;
@property(nonatomic, strong) NSNumber *displayToUse;
@property(nonatomic, strong) NSNumber *displayWidth;
@property(nonatomic, strong) NSNumber *displayHeight;
@property(nonatomic, strong) NSNumber *displayDistance;
@property(nonatomic) BOOL alwaysDisplayMirrorWindow;
@property(nonatomic, strong) NSNumber *mirrorWindowBaseHeight;
@property(nonatomic) BOOL announceIndividualStimuli;
@property(nonatomic) BOOL warnOnSkippedRefresh;
@property(nonatomic) BOOL allowAltFailover;

@end
