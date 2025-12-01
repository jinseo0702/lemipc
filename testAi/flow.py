import graphviz

# Create a directed graph
dot = graphviz.Digraph(comment='LemIPC Logic Flow', format='png')
dot.attr(rankdir='TB')

# Define node styles
dot.attr('node', shape='box', style='filled', fillcolor='white', fontname='Helvetica')

# --- 1. Initialization Phase ---
with dot.subgraph(name='cluster_init') as c:
    c.attr(label='1. Initialization', color='lightgrey')
    c.node('Start', 'Start Program\n(./lemipc <team>)', shape='oval', fillcolor='lightblue')
    c.node('IsFirst', 'Is First Player?', shape='diamond', fillcolor='lightyellow')
    
    c.node('CreateIPC', 'Create IPCs\n(Shm, Sem, MsgQ)\nInitialize Structs')
    c.node('AttachIPC', 'Attach IPCs\nCheck Team Count')
    
    c.node('JoinGame', 'Determine Position (x,y)\nWrite to SHM')
    c.node('WaitStart', 'Wait for Game Start\n(MsgQ / Player Count)')
    
    c.edge('Start', 'IsFirst')
    c.edge('IsFirst', 'CreateIPC', label='Yes')
    c.edge('IsFirst', 'AttachIPC', label='No')
    c.edge('CreateIPC', 'JoinGame')
    c.edge('AttachIPC', 'JoinGame')
    c.edge('JoinGame', 'WaitStart')

# --- 2. Main Loop Phase ---
with dot.subgraph(name='cluster_loop') as c:
    c.attr(label='2. Main Game Loop (Turn)', color='lightblue')
    
    c.node('LoopStart', 'Start Turn')
    c.node('CheckMsg', 'Check Message Queue')
    
    # Critical Section
    c.node('SemLock', 'Semaphore LOCK', style='filled', fillcolor='tomato', fontcolor='white')
    c.node('ReadBoard', 'Read Board Status')
    c.node('CheckEnemy', 'Enemy Nearby?', shape='diamond', fillcolor='lightyellow')
    
    c.node('ActionKill', 'Update SHM\n(Remove Enemy)\nSet Death Flag')
    c.node('ActionMove', 'Move Position\n(Algorithm)', shape='box')
    
    c.node('SemUnlock', 'Semaphore UNLOCK', style='filled', fillcolor='green', fontcolor='white')
    
    c.node('CheckFlag', 'Msg Flag ON?', shape='diamond')
    c.node('SendMsg', 'Send Message (MsgQ)')
    
    # Loop Edges
    c.edge('WaitStart', 'LoopStart')
    c.edge('LoopStart', 'CheckMsg')
    c.edge('CheckMsg', 'SemLock')
    c.edge('SemLock', 'ReadBoard')
    c.edge('ReadBoard', 'CheckEnemy')
    
    c.edge('CheckEnemy', 'ActionKill', label='Yes')
    c.edge('CheckEnemy', 'ActionMove', label='No')
    
    c.edge('ActionKill', 'SemUnlock')
    c.edge('ActionMove', 'SemUnlock')
    
    c.edge('SemUnlock', 'CheckFlag')
    c.edge('CheckFlag', 'SendMsg', label='Yes')
    c.edge('CheckFlag', 'LoopStart', label='No')
    c.edge('SendMsg', 'LoopStart')

# --- 3. Termination Phase ---
with dot.subgraph(name='cluster_end') as c:
    c.attr(label='3. Termination', color='lightgrey')
    
    c.node('CheckDeath', 'Death/End Signal?', shape='diamond', fillcolor='lightyellow')
    c.node('IsLast', 'Is Last Player?', shape='diamond', fillcolor='lightyellow')
    c.node('CleanIPC', 'Delete IPC Resources', style='filled', fillcolor='lightgrey')
    c.node('Exit', 'Exit Process', shape='oval', fillcolor='black', fontcolor='white')
    
    # Edges from loop
    # Implicit logic: Loop breaks on death/end
    dot.edge('CheckFlag', 'CheckDeath', label='(Check Health/End)')
    
    c.edge('CheckDeath', 'IsLast', label='Yes')
    c.edge('CheckDeath', 'LoopStart', label='No (Continue)')
    
    c.edge('IsLast', 'CleanIPC', label='Yes')
    c.edge('IsLast', 'Exit', label='No')
    c.edge('CleanIPC', 'Exit')

# Render
dot.render('lemipc_flowchart', cleanup=True)