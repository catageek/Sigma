@player
#1
&FPSCamera
>x=0.5f
>y=1.0f
>z=0.0f
>ry=180.0f

@stars
#6
&GLCubeSphere
#1
>scale=1.0f
>texture=assets/starss
>shader=shaders/skyboxs
>subdivision_levels=1i
>fix_to_camera=trueb
>cullface=fronts
>lightEnabled=0b

@planet1
#2
&GLIcoSphere
#1
>x=-10000.0f
>scale=1000.0f
>subdivisions=6i
>lightEnabled=0b

&PhysicalWorldLocation
>x=-10000.0f

@planet2
#3
&GLIcoSphere
#1
>x=-500.0f
>y=0.0f
>z=8000.0f
>scale=1000.0f
>lightEnabled=0b

&PhysicalWorldLocation
>x=-500.0f
>y=0.0f
>z=8000.0f

@planet3
#4
&GLIcoSphere
#1
>x=-500.0f
>y=0.0f
>z=2000.0f
>scale=500.0f
>lightEnabled=0b

&PhysicalWorldLocation
>x=-500.0f
>y=0.0f
>z=2000.0f

@ship
#5
&GLMesh
>scale=0.1f
>z=-3.0f
>y=0.75f
>x=-18.0f
>meshFile=shipobj/ship.objs
>cullface=nones
>shader=shaders/mesh_deferreds

&PhysicalWorldLocation
>z=-3.0f
>y=0.75f
>x=-18.0f

&RigidBody
>shape=meshs
>scale=0.1f
>meshFile=shipobj/ship.objs

@mars
#7
&GLCubeSphere
>scale=4000.0f
>x=0.0f
>y=-4000.0001f
>rz=45.0f
>ry=180.0f
>texture=assets/marss
>subdivision_levels=6i
>shader=shaders/cubespheres
>cull_face=backs
>lightEnabled=0b

&PhysicalWorldLocation
>x=0.0f
>y=-4000.0001f
>rz=45.0f
>ry=180.0f

&RigidBody
>shape=spheres
>scale=4000.0f
>radius=4000.0f

@hallLight1
#8
&PointLight
>y=1.5f
>z=5.0f
>radius=3.5f
>cr=0.0f
>cg=1.0f
>cb=0.0f

&PhysicalWorldLocation
>y=1.5f
>z=5.0f

@hallLight2
#9
&PointLight
>x=-5.0f
>y=1.5f
>z=5.0f
>radius=3.0f
>cr=1.0f
>cg=0.0f
>cb=0.0f

&PhysicalWorldLocation
>x=-5.0f
>y=1.5f
>z=5.0f

@hallLight3
#50
&PointLight
>x=5.0f
>y=1.5f
>z=5.0f
>radius=2.0f
>cr=0.0f
>cg=0.0f
>cb=1.0f

&PhysicalWorldLocation
>x=5.0f
>y=1.5f
>z=5.0f

@hallLight4
#51
&PointLight
>x=5.0
>y=1.5f
>z=0.0f
>radius=4.0f
>cr=1.0f
>cg=1.0f
>cb=1.0f

&PhysicalWorldLocation
>x=5.0
>y=1.5f
>z=0.0f

@hallLight5
#52
&PointLight
>x=-5.0f
>y=1.5f
>z=0.0f
>radius=3.0f
>cr=1.0f
>cg=0.0f
>cb=0.5f

&PhysicalWorldLocation
>x=-5.0f
>y=1.5f
>z=0.0f

@hallLight6
#53
&PointLight
>x=10.0f
>y=1.5f
>z=5.0f
>radius=2.0f
>cr=0.0f
>cg=1.0f
>cb=1.0f

&PhysicalWorldLocation
>x=10.0f
>y=1.5f
>z=5.0f

@hallLight7
#54
&PointLight
>x=10.0f
>y=1.5f
>z=0.0f
>radius=4.0f
>cr=0.5f
>cg=1.0f
>cb=0.5f

&PhysicalWorldLocation
>x=10.0f
>y=1.5f
>z=0.0f

@hallLight8
#55
&PointLight
>x=-10.0f
>y=1.5f
>z=5.0f
>radius=3.0f
>cr=1.0f
>cg=0.33f
>cb=0.66f

&PhysicalWorldLocation
>x=-10.0f
>y=1.5f
>z=5.0f

@hallLight9
#56
&PointLight
>x=-18.0f
>y=1.5f
>z=-5.0f
>radius=8.0f
>cr=1.0f
>cg=0.6f
>cb=0.4f

&PhysicalWorldLocation
>x=-18.0f
>y=1.5f
>z=-5.0f

@hallLight10
#57
&PointLight
>x=-18.0f
>y=1.5f
>z=-1.0f
>radius=8.0f
>cr=1.0f
>cg=0.6f
>cb=0.4f

&PhysicalWorldLocation
>x=-18.0f
>y=1.5f
>z=-1.0f

@engineLight1
#58
&PointLight
>x=-15.3f
>y=0.7f
>z=-3.25f
>radius=0.6f;
>cr=0.0f
>cg=0.1f
>cb=1.0f

&PhysicalWorldLocation
>x=-15.3f
>y=0.7f
>z=-3.25f

@engineLight2
#59
&PointLight
>x=-15.3f
>y=0.7f
>z=-2.75f
>radius=0.6f;
>cr=0.0f
>cg=0.1f
>cb=1.0f

&PhysicalWorldLocation
>x=-15.3f
>y=0.7f
>z=-2.75f

@center
#10
&GLMesh
>scale=1.0f
>ry=180.0f
>meshFile=Rooms/Room1/room1.objs
>cullface=nones
>shader=shaders/mesh_deferreds

&PhysicalWorldLocation
>ry=180.0f

&RigidBody
>meshFile=Rooms/Room1/room1.objs
>shape=meshs
>scale=1.0f

@northtway
#11
&GLMesh
>scale=1.0f
>z=5.0f
>ry=180.0f
>meshFile=Rooms/Room2/room2.objs
>cullface=nones
>shader=shaders/mesh_deferreds

&PhysicalWorldLocation
>z=5.0f
>ry=180.0f

&RigidBody
>shape=meshs
>scale=1.0f
>meshFile=Rooms/Room2/room2.objs

@flashlight
#150
&GLMesh
>scale=0.3f
>x=0.1f
>y=-0.15f
>z=-0.2f
>rx=-90.0f
>meshFile=flashlight/flashlight2.objs
>shader=shaders/mesh_deferreds
>cullface=nones
>parent=1i

&PhysicalWorldLocation
>x=0.1f
>y=-0.15f
>z=-0.2f
>rx=-90.0f

@flashlight_light
#151
&SpotLight
>z=0.0f
>y=1.0f
>x=0.0f
>rx=90.0f
>ry=0.0f
>rz=0.0f
>cr=1.0f
>cg=1.0f
>cb=1.0f
>angle=0.45f
>parent=150i

&PhysicalWorldLocation
>z=0.0f
>y=1.0f
>x=0.0f
>rx=90.0f
>ry=0.0f
>rz=0.0f

@northtwayeastexit
#12
&GLMesh
>scale=1.0f
>z=5.0f
>x=-5.0f
>meshFile=Rooms/Room5/room5.objs
>cullface=nones
>shader=shaders/mesh_deferreds

&PhysicalWorldLocation
>z=5.0f
>x=-5.0f

&RigidBody
>shape=meshs
>scale=1.0f
>meshFile=Rooms/Room5/room5.objs

@easttway
#13
&GLMesh
>scale=1.0f
>x=-5.0f
>ry=90.0f
>meshFile=Rooms/Room6/room6.objs
>cullface=nones
>shader=shaders/mesh_deferreds

&PhysicalWorldLocation
>x=-5.0f
>ry=90.0f

&RigidBody
>shape=meshs
>scale=1.0f
>meshFile=Rooms/Room6/room6.objs

@easttwayeastexit
#14
&GLMesh
>scale=1.0f
>x=-10.0f
>meshFile=Rooms/Room4/room4.objs
>cullface=nones
>shader=shaders/mesh_deferreds

&PhysicalWorldLocation
>x=-10.0f

&RigidBody
>shape=meshs
>scale=1.0f
>meshFile=Rooms/Room4/room4.objs

@easthanger
#15
&GLMesh
>scale=1.0f
>x=-20.0f
>meshFile=Rooms/Hanger/hanger.objs
>cullface=nones
>shader=shaders/mesh_deferreds

&PhysicalWorldLocation
>x=-20.0f

&RigidBody
>shape=meshs
>scale=1.0f
>meshFile=Rooms/Hanger/hanger.objs

@easttwaysouthexit
#16
&GLMesh
>scale=1.0f
>z=-5.0f
>x=-5.0f
>ry=180f
>meshFile=Rooms/Room3/room3.objs
>cullface=nones
>shader=shaders/mesh_deferreds

&PhysicalWorldLocation
>z=-5.0f
>x=-5.0f
>ry=180f

&RigidBody
>shape=meshs
>scale=1.0f
>meshFile=Rooms/Room3/room3.objs

@easthangersouthexit
#17
&GLMesh
>scale=1.0f
>z=-5.0f
>x=-10.0f
>meshFile=Rooms/Room3/room3.objs
>cullface=nones
>shader=shaders/mesh_deferreds

&PhysicalWorldLocation
>z=-5.0f
>x=-10.0f

&RigidBody
>shape=meshs
>scale=1.0f
>meshFile=Rooms/Room3/room3.objs

@eastmostcornerhallway
#18
&GLMesh
>scale=1.0f
>z=-10.0f
>x=-10.0f
>ry=270.0f
>meshFile=Rooms/Room5/room5.objs
>cullface=nones
>shader=shaders/mesh_deferreds

&PhysicalWorldLocation
>z=-10.0f
>x=-10.0f
>ry=270.0f

&RigidBody
>shape=meshs
>scale=1.0f
>meshFile=Rooms/Room5/room5.objs

@southeasthallway
#19
&GLMesh
>scale=1.0f
>z=-10.0f
>x=-5.0f
>meshFile=Rooms/Room4/room4.objs
>cullface=nones
>shader=shaders/mesh_deferreds

&PhysicalWorldLocation
>z=-10.0f
>x=-5.0f

&RigidBody
>shape=meshs
>scale=1.0f
>meshFile=Rooms/Room4/room4.objs

@souththallway
#20
&GLMesh
>scale=1.0f
>z=-5.0f
>meshFile=Rooms/Room6/room6.objs
>cullface=nones
>shader=shaders/mesh_deferreds

&PhysicalWorldLocation
>z=-5.0f

&RigidBody
>shape=meshs
>scale=1.0f
>meshFile=Rooms/Room6/room6.objs

@south4way
#21
&GLMesh
>scale=1.0f
>z=-10.0f
>meshFile=Rooms/Room7/room7.objs
>cullface=nones
>shader=shaders/mesh_deferreds

&PhysicalWorldLocation
>z=-10.0f

&RigidBody
>shape=meshs
>scale=1.0f
>meshFile=Rooms/Room7/room7.objs

@northtwaywestexit
#22
&GLMesh
>scale=1.0f
>z=5.0f
>x=5.0f
>ry=90.0f
>meshFile=Rooms/Room5/room5.objs
>cullface=nones
>shader=shaders/mesh_deferreds

&PhysicalWorldLocation
>z=5.0f
>x=5.0f
>ry=90.0f

&RigidBody
>shape=meshs
>scale=1.0f
>meshFile=Rooms/Room5/room5.objs

@westtway
#23
&GLMesh
>scale=1.0f
>x=5.0f
>ry=270.0f
>meshFile=Rooms/Room6/room6.objs
>cullface=nones
>shader=shaders/mesh_deferreds

&PhysicalWorldLocation
>x=5.0f
>ry=270.0f

&RigidBody
>shape=meshs
>scale=1.0f
>meshFile=Rooms/Room6/room6.objs

@westtwaywestexit
#24
&GLMesh
>scale=1.0f
>x=10.0f
>meshFile=Rooms/Room4/room4.objs
>cullface=nones
>shader=shaders/mesh_deferreds

&PhysicalWorldLocation
>x=10.0f

&RigidBody
>shape=meshs
>scale=1.0f
>meshFile=Rooms/Room4/room4.objs

@westhanger
#25
&GLMesh
>scale=1.0f
>z=-5.0f
>x=20.0f
>ry=180.0f
>meshFile=Rooms/Hanger/hanger.objs
>cullface=nones
>shader=shaders/mesh_deferreds

&PhysicalWorldLocation
>z=-5.0f
>x=20.0f
>ry=180.0f

&RigidBody
>shape=meshs
>scale=1.0f
>meshFile=Rooms/Hanger/hanger.objs

@westtwaysouthexit
#26
&GLMesh
>scale=1.0f
>z=-5.0f
>x=5.0f
>ry=90.0f
>meshFile=Rooms/Room3/room3.objs
>cullface=nones
>shader=shaders/mesh_deferreds

&PhysicalWorldLocation
>z=-5.0f
>x=5.0f
>ry=90.0f

&RigidBody
>shape=meshs
>scale=1.0f
>meshFile=Rooms/Room3/room3.objs

@westhangersouthexit
#27
&GLMesh
>scale=1.0f
>z=-5.0f
>x=10.0f
>ry=270.0f
>meshFile=Rooms/Room3/room3.objs
>cullface=nones
>shader=shaders/mesh_deferreds

&PhysicalWorldLocation
>z=-5.0f
>x=10.0f
>ry=270.0f

&RigidBody
>shape=meshs
>scale=1.0f
>meshFile=Rooms/Room3/room3.objs

@westmostcornerhallway
#28
&GLMesh
>scale=1.0f
>z=-10.0f
>x=10.0f
>ry=180.0f
>meshFile=Rooms/Room5/room5.objs
>cullface=nones
>shader=shaders/mesh_deferreds

&PhysicalWorldLocation
>z=-10.0f
>x=10.0f
>ry=180.0f

&RigidBody
>shape=meshs
>scale=1.0f
>meshFile=Rooms/Room5/room5.objs

@southwesthallway
#29
&GLMesh
>scale=1.0f
>z=-10.0f
>x=5.0f
>meshFile=Rooms/Room4/room4.objs
>cullface=nones
>shader=shaders/mesh_deferreds

&PhysicalWorldLocation
>z=-10.0f
>x=5.0f

&RigidBody
>shape=meshs
>scale=1.0f
>meshFile=Rooms/Room4/room4.objs

@airlock
#30
&GLMesh
>scale=1.0f
>z=-15.0f
>ry=90.0f
>meshFile=Rooms/Room9/room9.objs
>cullface=nones
>shader=shaders/mesh_deferreds

&PhysicalWorldLocation
>z=-15.0f
>ry=90.0f

&RigidBody
>shape=meshs
>scale=1.0f
>meshFile=Rooms/Room9/room9.objs

@MonitorChassis
#31
&GLMesh
>scale=0.1f
>z=1.2f
>x=1.7f
>ry=270.0f
>meshFile=vidstand/VidStand.objs
>shader=shaders/mesh_deferreds
>lightEnabled=0b

&PhysicalWorldLocation
>z=1.2f
>x=1.7f
>ry=270.0f

@MonitorPannel
#32
&WebGUIView
>left=0.0f
>top=0.0f
>width=1.0f
>height=1.0f
>textureName=monitors
>URL=http://www.google.com/s

&GLMesh
>scale=0.1f
>z=1.2f
>x=1.7f
>ry=270.0f
>meshFile=vidstand/VidStand_out.objs
>shader=shaders/mesh_deferreds
>lightEnabled=0b

&PhysicalWorldLocation
>z=1.2f
>x=1.7f
>ry=270.0f

@soundtest
#200
&ALSound
>x=0.0f
>y=0.0f
>z=0.0f
>soundFilename=theme.oggs
